#pragma once

#include <eosio/eosio.hpp>
#include "../models/identity.h"
#include "../models/config.h"
#include "../lib/publickey.h"
#include "../../ridl_token/ridl_token.hpp"

using namespace eosio;
using namespace identity;
using namespace config;

class IdentityActions {

private:
    name _self;
    Identities identities;
    Topups topups;

    auto findIdentity(string username){
        lower(username);
        auto index = identities.get_index<"name"_n>();
        auto found = index.find(toUUID(username));
        check(found != index.end(), ("Could not find username: "+username).c_str());
        return identities.find(found->id);
    }

public:
    IdentityActions(name self):_self(self),
        identities(_self, _self.value),
        topups(_self, _self.value) {}



    void identify(string& username, const public_key& key){
        require_auth(Configs(_self, _self.value).get().identity_creator);
        Identity identity = Identity::create(username, key);
        createIdentity(identity);
    }

    void buyactions(uuid& identity_id){
        require_auth(Configs(_self, _self.value).get().identity_creator);
        auto identity = identities.find(identity_id);
        check(identity != identities.end(), "Identity does not exist");

        asset maxCanHold = asset(100'0000 + (identity->expansion.amount > 0 ? identity->expansion.amount : 0), S_RIDL);

        identities.modify(identity, same_payer, [&](auto& record){
            record.tokens = maxCanHold;
        });

    }

    void revoke(uuid& identity_id){
        require_auth(Configs(_self, _self.value).get().identity_creator);
        auto identity = identities.find(identity_id);
        check(identity != identities.end(), "Identity does not exist");

        check(!identity->activated, "This identity can no longer be revoked.");
        identities.erase(identity);
    }

    void activate(uuid& identity_id){
        require_auth(Configs(_self, _self.value).get().identity_creator);
        auto identity = identities.find(identity_id);
        check(identity != identities.end(), "Identity does not exist");

        check(!identity->activated, "This identity is already activated.");
        
        identities.modify(identity, same_payer, [&](auto& record){
            record.activated = 1;
        });
    }


    void changekey(uuid& identity_id, const string& key, uint64_t& block_num, const signature& sig){
        auto identity = identities.find(identity_id);
        check(identity != identities.end(), "Identity does not exist");
        check(identity->activated, "Your identity is not yet activated, please wait.");

        checkBlockNum(block_num);

        // Example `1:201239:EOS8YQzaYLxT17fWAPueQBxRjHehTQYZEvgPAWPPH4mAuwTJi3mPN`
        string cleartext = std::to_string(identity->id) + ":" + std::to_string(block_num) + ":" + key;
        checksum256 hash = sha256(cleartext.c_str(), cleartext.size());
        assert_recover_key(hash, sig, identity->key);

        identities.modify(identity, same_payer, [&](auto& record){
            record.key = getPublicKey(key);
        });
    }


    void seed(string& username, const public_key& key){
        require_auth(_self);

        Identity identity = Identity::create(username, key);

        createIdentity(identity, 100'0000, 1);
    }

    void loadtokens(const name& from, const asset& tokens, const string& memo){
        auto identity = findIdentity(memo);
        check(identity->activated, "Your identity is not yet activated, please wait.");
        auto existing = topups.find(identity->fingerprint);

        check(tokens.symbol == S_RIDL, "Can only load RIDL tokens into an identity.");

        asset maxCanHold = asset(100'0000 + (identity->expansion.amount > 0 ? identity->expansion.amount : 0), S_RIDL);
        asset total = tokens + identity->tokens;
        if(existing != topups.end()) total += existing->tokens;
        asset remaining = tokens;

        // Too much was spent, sending the overage back
        if(total > maxCanHold){
            asset refunded = total - maxCanHold;
            remaining = remaining - refunded;
            sendRIDL(_self, from, refunded, "RIDL usage token overload remainder");
        }

        check(remaining.amount > 0, "Identity is already holding its maximum RIDL usage token capacity.");

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

        sendDeferred(_self, name("tokensloaded"), identity->id, TOPUP_DELAY, identity->fingerprint);
    }

    void tokensloaded(uuid& identity_id){
        auto identity = identities.find(identity_id);
        check(identity->activated, "Your identity is not yet activated, please wait.");
        check(identity != identities.end(), "An Identity with that name does not exist.");

        auto topup = topups.find(identity->fingerprint);
        check(topup != topups.end(), "There is no pending topup for this Identity.");
        check(topup->claimable >= now(), "This Identity's topup is not yet available for use.");

        identities.modify(identity, same_payer, [&](auto& record){
            record.tokens += topup->tokens;
        });

        topups.erase(topup);
    }

private:


    void createIdentity(Identity& identity, int64_t tokens = 20'0000, uint8_t preActivated = 0){
        auto index = identities.get_index<"name"_n>();
        auto found = index.find(toUUID(identity.username));
        check(found == index.end(), "This identity already exists.");

        identities.emplace(_self, [&](auto& record){
            identity.id = identities.available_primary_key();
            if(identity.id == 0) identity.id = 1;
            record = identity;
            record.tokens = asset(tokens, S_RIDL);
            record.block = tapos_block_num();
            record.activated = preActivated;

            // TODO: Need to move tokens from reserves to this contract.
        });
    }

};
