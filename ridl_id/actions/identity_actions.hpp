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
        require_auth(Configs(_self, 0).get().manager);
        Identity identity = Identity::create(username, key);
        createIdentity(identity);
    }

    void buyactions(const uuid& identity_id, asset& quantity){
        require_auth(Configs(_self, 0).get().manager);
        auto identity = identities.find(identity_id);
        check(identity != identities.end(), "Identity does not exist");
        loadtokens(*identity, quantity);
    }

    void movedtokens(string& username, asset& quantity){
        require_auth(Configs(_self, 0).get().manager);
        auto identity = findIdentity(username);
        check(identity != identities.end(), "Identity does not exist");
        loadtokens(*identity, quantity);
    }

    void revoke(uuid& identity_id){
        require_auth(Configs(_self, 0).get().manager);
        auto identity = identities.find(identity_id);
        check(identity != identities.end(), "Identity does not exist");

        check(!identity->activated, "This identity can no longer be revoked.");
        identities.erase(identity);
    }

    void activate(uuid& identity_id){
        require_auth(Configs(_self, 0).get().manager);
        auto identity = identities.find(identity_id);
        check(identity != identities.end(), "Identity does not exist");

        check(!identity->activated, "This identity is already activated.");
        
        identities.modify(identity, same_payer, [&](auto& record){
            record.activated = 1;
        });
    }

    void moveidentity(uuid& identity_id, string& new_chain_id){
        require_auth(Configs(_self, 0).get().manager);
        auto identity = identities.find(identity_id);
        check(identity != identities.end(), "Identity does not exist");
        identities.erase(identity);
    }


    void seed(string& username, const public_key& key, asset& tokens, asset& expansion){
        require_auth(Configs(_self, 0).get().manager);
        auto existing = findIdentity(username);
        check(existing == identities.end(), "Identity already exists");

        Identity identity = Identity::create(username, key);
        createIdentity(identity, tokens.amount, expansion.amount, 1);
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

    void loadTokensFromTransfer(const name& from, const asset& tokens, const string& memo){
        auto identity = findIdentity(memo);
        loadtokens(*identity, tokens);
    }

    void loadtokens(Identity identity, const asset& tokens){
        // Always tries to get loaded tokens before hand, which resets timer and possibly removes rows.
        tokensloaded(identity.id);

        check( tokens.symbol.is_valid(), "RIDL symbol is invalid." );
        check( tokens.is_valid(), "RIDL quantity is invalid." );
        check( tokens.amount >= 1, "Must load at least 1.0000 RIDL." ); // No loading dust
        check( tokens.symbol == S_RIDL, "Can only load RIDL tokens into an identity." );

        auto topup = topups.find(identity.id);
        if(topup == topups.end()) topups.emplace(_self, [&](auto& row){
            row.id = identity.id;
            row.tokens = tokens;
            row.claimable = now() + TOPUP_DELAY;
        });
        else topups.modify(topup, same_payer, [&](auto& row){
            row.tokens += tokens;
        });
    }

    void tokensloaded(const uuid& identity_id){
        // Shame that we have to waste CPU here to re-find identities, but
        // this could be (and is) called outside of the `loadtokens` scope.
        auto identity = identities.find(identity_id);
        check(identity->activated, "Your identity is not yet activated, please wait.");
        check(identity != identities.end(), "An Identity with that name does not exist.");

        auto topup = topups.find(identity->id);
        if(topup == topups.end()) return;
        // Don't merge end check and time check into one if-statement, EOSIO doesn't respect ordering and bills for all conditions.
//        if(topup->claimable < now()){
        if(true){
            asset spaceLeft = identity->spaceLeft();
            if(spaceLeft.amount > 0) {
                asset tokensToLoad((topup->tokens > spaceLeft ? spaceLeft : topup->tokens).amount, S_RIDL);

//                check(false, "Space left: " + spaceLeft.to_string() + ", tokens to load: " + tokensToLoad.to_string() + ", topup tokens: " + topup->tokens.to_string());
//                check(false, "Tokens left in topup: " + (topup->tokens - tokensToLoad).to_string());

                if ((topup->tokens - tokensToLoad).amount == 0) topups.erase(topup);
                else topups.modify(topup, same_payer, [&](auto &row) {
                    row.tokens -= tokensToLoad;
                    row.claimable = now() + TOPUP_DELAY;
                });

                identities.modify(identity, same_payer, [&](auto &record) {
                    record.tokens += tokensToLoad;
                });
            }
        }
    }

private:


    void createIdentity(Identity& identity, int64_t tokens = 20'0000, int64_t expansion = 1'0000, uint8_t preActivated = 0){
        auto index = identities.get_index<"name"_n>();
        auto found = index.find(toUUID(identity.username));
        check(found == index.end(), "This identity already exists.");

        identities.emplace(_self, [&](auto& record){
            identity.id = identities.available_primary_key();
            if(identity.id == 0) identity.id = 1;
            record = identity;
            record.expansion = asset(expansion, S_RIDL);
            record.tokens = asset(tokens, S_RIDL);
            record.block = tapos_block_num();
            record.activated = preActivated;

            // TODO: Need to move tokens from reserves to this contract.
        });
    }

};
