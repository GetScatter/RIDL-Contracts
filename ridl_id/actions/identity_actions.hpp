#pragma once

#include <eosiolib/eosio.hpp>
#include "../models/config.h"
#include "../models/identity.h"
#include "../models/reputation.h"
#include "../../ridl_token/ridl_token.hpp"

#include "./balance_actions.hpp"

using namespace eosio;
using namespace config;
using namespace identity;
using namespace reputation;

class IdentityActions {
public:
    name _self;
    IdentityActions(name self):_self(self){}

    // Identifies a new user or re-registers an existing user
    void identify(const name& account, string& username, const public_key& key){
        require_auth(account);

        // TODO: calc price from oracle/configs
        asset price(1'0000, S_EOS);

        BalanceActions(_self).hasBalance(account, price);

        // Constructing new identity
        Identity identity = identity::create(username, key, account);

        // Name is valid
        identity::validateName(identity.username);


        // Set Identity
        createOrUpdateIdentity(identity);

        BalanceActions(_self).subBalance(account, price);
//
//        string f = "user::"+username;
//        uuid repFingerprint = toUUID(f);
//
//        asset repTotal = RepTotal(_self, repFingerprint).get_or_default(asset(0'0000, S_REP));
//        RepTotal(_self, repFingerprint).set(repTotal, account);


        // TODO: Only allow repayment starting 4 months from end of payment year ( makes sure users can't pay for 10 years at once )
        // TODO: Allows users to repay their identity ( expires +1 year from previous ending date, not date of payment if renewed )
    }


    void release(string& username, const signature& sig){
        uuid fingerprint = toUUID(username);
        Identities identities(_self, _self.value);
        auto identity = identities.find(fingerprint);
        eosio_assert(identity != identities.end(), "Identity does not exist");

        require_auth(identity->account);
        common::prove(sig, identity->key);

        identities.erase(identity);

        // TODO: Remove all reputation + reputation mines owned by user
        // Some reputation pots will be locked in the user's RAM until the next user comes along if they
        // haven't gotten rid of them yet. The only other way would be to release them to the contract itself
        // but that could become an attack vector.
    }


    void rekey(string& username, const public_key& key){
        uuid fingerprint = toUUID(username);
        Identities identities(_self, _self.value);
        auto identity = identities.find(fingerprint);
        eosio_assert(identity != identities.end(), "Identity does not exist");

        require_auth(identity->account);

        identities.modify(identity, _self, [&](auto& record){
            record.key = key;
        });
    }


    void setaccountacc(string& username, const name& new_account){
        lower(username);
        uuid fingerprint = toUUID(username);

        Identities identities(_self, _self.value);
        auto identity = identities.find(fingerprint);
        eosio_assert(identity != identities.end(), "Identity does not exist");
        require_auth(identity->account);

        identities.modify(identity, _self, [&](auto& record){
            record.account = new_account;
        });
    }

    void setaccountkey(string& username, const name& new_account, const signature& sig){
        lower(username);
        uuid fingerprint = toUUID(username);

        Identities identities(_self, _self.value);
        auto identity = identities.find(fingerprint);
        eosio_assert(identity != identities.end(), "Identity does not exist");
        common::prove(sig, identity->key);

        identities.modify(identity, _self, [&](auto& record){
            record.account = new_account;
        });
    }


//    // TODO: Bid on an existing identity
//    void bid(string& username, name& bidder, asset& bid){}
//
//    // TODO: Transfers an Identity ( Should be multi-sig so the recipient can claim using their identity's key )
//    void sell(string& username, name& owner, uuid& bid_id){}
//
//
    void seed(string& username, const public_key& key){
        require_auth(_self);

        Identity identity = identity::create(username, key, _self);
        identity.expires = now() + ((seconds_per_day * 365) * 100);
        createOrUpdateIdentity(identity, 100'0000);
    }


    void claim(const name& account, string& username, const public_key& key, const signature& sig){
        require_auth(account);

        lower(username);
        uuid fingerprint = toUUID(username);

        Identities identities(_self, _self.value);
        auto identity = identities.find(fingerprint);
        eosio_assert(identity != identities.end(), "Identity not seeded.");
        eosio_assert(identity->account == _self, "Identity already owned.");

        // Proving ownership of public key
        common::prove(sig, key);

        identities.modify(identity, _self, [&](auto& record){
            record.account = account;
            record.key = key;
        });
    }

    void loadtokens(const name& account, string& username, const asset& tokens){
        require_auth(account);
        eosio_assert(tokens.symbol == S_RIDL, "Can only load RIDL tokens into an identity.");

        BalanceActions(_self).hasBalance(account, tokens);
        BalanceActions(_self).subBalance(account, tokens);

        lower(username);
        uuid fingerprint = toUUID(username);
        Identities identities(_self, _self.value);
        auto identity = identities.find(fingerprint);
        eosio_assert(identity != identities.end(), "Cannot transfer RIDL tokens to an Identity that does not exist");


        uint64_t reputationBonus = 100'0000 + (identity->total_rep.amount > 0 ? identity->total_rep.amount : 0);
        asset maxCanHold = asset(reputationBonus, S_RIDL);

        eosio_assert(identity->tokens < maxCanHold, "Already holding maximum RIDL in Identity");

        asset quantity = tokens + identity->tokens;

        // Too much was spent, sending the overage back
        if(quantity > maxCanHold){
            asset refunded = quantity - maxCanHold;
            quantity = quantity - refunded;
            INLINE_ACTION_SENDER(ridl::token, transfer)( "ridlridlcoin"_n, {{_self,"active"_n}}, {_self, account, refunded, "RIDL Over-Loaded Refund"} );
        }

        eosio_assert(quantity.amount > 0, "Identity is already holding max RIDL");

        identities.modify(identity, _self, [&](auto& record){
            record.tokens = quantity;
        });
    }

//
//
//    void loadedtokens( const transfer& t ){
//        // Token Assertions
//        eosio_assert(t.quantity.symbol == S_RIDL, "Token must be RIDL");
//        eosio_assert(t.quantity.is_valid(), "Token asset is not valid");
//        eosio_assert(t.quantity.amount > 0, "Not enough tokens");
//
//        // Memo Assertions
//        eosio_assert(t.memo.length() > 0, "Memo can not be empty");
//        std::vector<string> params = memoToApiParams(t.memo);
//        eosio_assert(params.size() == 2, "Memo must have 2 parameters");
//
//        name account = name(params[0].c_str());
//        string username = params[1].c_str();
//
//        lower(username);
//        uuid fingerprint = toUUID(username);
//        Identities identities(_self, _self.value);
//        auto identity = identities.find(fingerprint);
//        eosio_assert(identity != identities.end(), "Cannot transfer RIDL tokens to an Identity that does not exist");
//
//        asset quantity = t.quantity + identity->tokens;
//
//        uint64_t reputationBonus = 100'0000 + RepTotal(_self, fingerprint).get_or_default(asset(0'0000, S_REP)).amount;
//        asset maxCanHold = asset(reputationBonus, S_RIDL);
//
//        // Too much was spent, sending the overage back
//        if(quantity > maxCanHold){
//            asset refunded = quantity - maxCanHold;
//            quantity = quantity - refunded;
//            INLINE_ACTION_SENDER(ridl::token, transfer)( "ridlridlcoin"_n, {{_self,"active"_n}}, {_self, t.from, refunded, "RIDL Over-Loaded Refund"} );
//        }
//
//        eosio_assert(quantity.amount > 0, "Identity is already holding max RIDL");
//
//        identities.modify(identity, _self, [&](auto& record){
//            record.tokens = quantity;
//        });
//    }
//

private:


    void createOrUpdateIdentity(Identity& identity, int64_t tokens = 20'0000){
        Identities identities(_self, _self.value);
        auto existingIdentity = identities.find(identity.fingerprint);

        if(existingIdentity != identities.end()){
            bool isExpired = now() > existingIdentity->expires;
            bool isSameOwner = existingIdentity->account == identity.account;

            if(!isSameOwner){
                eosio_assert(!isExpired, "Identity exists and still is still being paid for.");
            } else {
                uint64_t threeMonthsFromEndingPeriod = existingIdentity->expires - (seconds_per_day * 90);
                eosio_assert(isExpired || now() > threeMonthsFromEndingPeriod, "Can only re-purchase Identity for yourself within the last 90 days");

                if(!isExpired){
                    // Adding 365 days to the last expiration date
                    identity.expires = existingIdentity->expires + (seconds_per_day * 365);
                }
            }
        }


        // Registering new identity
        if(existingIdentity == identities.end()){
            identities.emplace(_self, [&](auto& record){
                record = identity;
                record.tokens = asset(tokens, S_RIDL);

                // TODO: Need to move tokens from reserves to this contract.
            });
        }

        // Updating expired identity
        else {
            //TODO: If identity is not the same account owner need to replace reputation.

            identities.modify(existingIdentity, _self, [&](auto& record){
                if(existingIdentity->account == identity.account){
                    record.expires = identity.expires;
                } else {
                    record = identity;
                }
            });
        }
    }

};
