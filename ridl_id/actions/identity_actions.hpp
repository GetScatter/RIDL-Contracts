#pragma once

#include <eosiolib/eosio.hpp>
#include <eosiolib/transaction.hpp>
#include "../models/identity.h"
#include "../models/reputation.h"
#include "../../ridl_token/ridl_token.hpp"

#include "./balance_actions.hpp"

using namespace eosio;
using namespace identity;
using namespace reputation;

class IdentityActions {

private:
    name _self;
    Identities identities;
    Topups topups;

    auto findIdentity(string& username){
        lower(username);
        auto index = identities.get_index<"name"_n>();
        auto found = index.find(toUUID(username));
        eosio_assert(found != index.end(), ("Could not find username: "+username).c_str());
        return identities.find(found->id);
    }

public:
    IdentityActions(name self):_self(self),
        identities(_self, _self.value),
        topups(_self, _self.value) {}

    // Identifies a new user or re-registers an existing user
    void identify(const name& account, string& username, const public_key& key){
        require_auth(account);

        // TODO: calc price from oracle/configs
        asset price(1'0000, S_EOS);

        // Constructing new identity
        Identity identity = Identity::create(username, key, account);

        // Set Identity
        createOrUpdateIdentity(identity);

        BalanceActions(_self).subBalance(account, price);
//
//        string f = "user::"+username;
//        uuid repFingerprint = toUUID(f);
//
//        asset repTotal = RepTotal(_self, repFingerprint).get_or_default(asset(0'0000, S_REP));
//        RepTotal(_self, repFingerprint).set(repTotal, account);


    }


    void changekey(string& username, const public_key& key){
        auto identity = findIdentity(username);
        eosio_assert(identity != identities.end(), "Identity does not exist");
        identity->authenticate();

        identities.modify(identity, _self, [&](auto& record){
            record.key = key;
        });
    }


    void changeacc(string& username, name& account){
        eosio_assert(account != _self, "Can't reset account to ridlridlridl");
        auto identity = findIdentity(username);
        eosio_assert(identity != identities.end(), "Identity does not exist");
        identity->authenticate();

        identities.modify(identity, _self, [&](auto& record){
            record.account = account;
        });
    }


    void seed(string& username, const public_key& key){
        require_auth(_self);

        Identity identity = Identity::create(username, key, _self);
        identity.expires = now() + ((SECONDS_PER_DAY * 365) * 100);
        createOrUpdateIdentity(identity, 100'0000);
    }


    void claim(const name& account, string& username, const public_key& key, const signature& sig){
        require_auth(account);

        auto identity = findIdentity(username);
        eosio_assert(identity != identities.end(), "Identity not seeded.");
        eosio_assert(identity->account == _self, "Identity already owned.");

        // Proving ownership of public key
        common::prove(sig, key);

        identities.modify(identity, _self, [&](auto& record){
            record.account = account;
            record.key = key;
        });
    }

    void loadtokens(string& username, const asset& tokens){
        eosio_assert(tokens.symbol == S_RIDL, "Can only load RIDL tokens into an identity.");

        auto identity = findIdentity(username);
        eosio_assert(identity != identities.end(), "Cannot transfer RIDL tokens to an Identity that does not exist");
        identity->authenticate();

        // TODO: ADD IF EXISTING
        auto existing = topups.find(identity->fingerprint);

        asset maxCanHold = asset(100'0000 + (identity->total_rep.amount > 0 ? identity->total_rep.amount : 0) + (identity->expansion.amount > 0 ? identity->expansion.amount : 0), S_RIDL);
        asset total = tokens + identity->tokens;
        if(existing != topups.end()) total += existing->tokens;
        asset remaining = tokens;

        // Too much was spent, sending the overage back
        if(total > maxCanHold){
            asset refunded = total - maxCanHold;
            remaining = remaining - refunded;
            sendRIDL(_self, identity->account, refunded, "RIDL overload refund");
        }

        eosio_assert(remaining.amount > 0, "Identity is already holding it's maximum RIDL capacity.");
        BalanceActions(_self).subBalance(identity->account, tokens);

        if(existing == topups.end()){
            topups.emplace(_self, [&](auto& row){
                row.fingerprint = identity->fingerprint;
                row.tokens = remaining;
                row.claimable = now() + TOPUP_DELAY;
            });
        } else {
            topups.modify(existing, same_payer, [&](auto& row){
                row.tokens += remaining;
                row.claimable = now() + TOPUP_DELAY;
            });
        }

        // Temporary fix to cancel deferred transactions because cancelling them from the .send()
        // method throws an exception temporarily due to a patched but unapplied RAM exploit
        // https://github.com/EOSIO/eos/issues/6541
        cancel_deferred(identity->fingerprint);

        transaction t;
        t.actions.emplace_back(action( permission_level{ _self, "active"_n }, _self, name("tokensloaded"), make_tuple(username)));
        t.delay_sec = TOPUP_DELAY;
        t.send(identity->fingerprint, _self, true);
    }

    void tokensloaded(string& username){
        auto identity = findIdentity(username);
        eosio_assert(identity != identities.end(), "An Identity with that name does not exist.");

        auto topup = topups.find(identity->fingerprint);
        eosio_assert(topup != topups.end(), "There is no pending topup for this Identity.");
        eosio_assert(topup->claimable >= now(), "This Identity's topup is not yet available for use.");

        identities.modify(identity, _self, [&](auto& record){
            record.tokens += topup->tokens;
        });

        topups.erase(topup);
    }

private:


    void createOrUpdateIdentity(Identity& identity, int64_t tokens = 20'0000){
        auto existingIdentity = identities.find(identity.fingerprint);

        auto insertNewIdentity = [&](){
            identities.emplace(_self, [&](auto& record){
                identity.id = identities.available_primary_key();
                record = identity;
                record.tokens = asset(tokens, S_RIDL);

                // TODO: REMOVE OLD IDENTITY REPUTATION IF EXISTS
                // TODO: Need to move tokens from reserves to this contract.
            });
        };

        ///////////////////////////////////
        // Repurchasing or new owner for existing identity
        if(existingIdentity != identities.end()){
            bool isExpired = now() > existingIdentity->expires;
            bool isSameOwner = existingIdentity->account == identity.account;
            uint64_t canRepurchase = now() > existingIdentity->expires - (SECONDS_PER_DAY * 90);

            eosio_assert(isExpired || isSameOwner, ("Identity "+identity.username+" is already taken.").c_str());
            eosio_assert(isExpired || (isSameOwner && canRepurchase), "Can only re-purchase Identity for yourself within the last 90 days.");

            if(isSameOwner){
                identities.modify(existingIdentity, _self, [&](auto& record){
                    record.expires = existingIdentity->expires + (SECONDS_PER_DAY * 365);
                });
            } else {
                identities.erase(existingIdentity);
                insertNewIdentity();
            }


        }

        // Registering new identity
        else insertNewIdentity();
    }

};
