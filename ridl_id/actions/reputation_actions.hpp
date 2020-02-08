#pragma once

#include <eosio/eosio.hpp>
#include <eosio/crypto.hpp>
#include "../models/identity.h"
#include "../models/reputation.h"
#include "../../ridl_token/ridl_token.hpp"

using namespace eosio;
using namespace identity;
using namespace reputation;

using std::string;
using std::vector;
using std::make_tuple;

class ReputationActions {
private:
    name _self;
    Identities identities;

    auto findIdentity(string& username){
        lower(username);
        auto index = identities.get_index<"name"_n>();
        auto found = index.find(toUUID(username));
        check(found != index.end(), ("Could not find username: "+username).c_str());
        return identities.find(found->id);
    }

public:
    ReputationActions(name self):_self(self),
        identities(_self, _self.value){}

    void repute(uuid& identity_id, string& entity, vector<ReputationFragment>& fragments, asset& tokens, uint64_t& block_num, const signature& sig, name& sender){
        if(is_account(sender)) require_auth(sender);

        // Token checks
        check(tokens.is_valid(), "tokens parameter is not valid (Not within limits)" );
        check(tokens.symbol == S_RIDL, "tokens precision mismatch (Should be 4 decimals)" );

        ///////////////////////////////////
        // Assertions and formatting
        check(entity.size() > 0, "Entity is invalid");
        check(fragments.size() > 0, "You must repute at least one fragment");
        // END /////////////////////////////////


        ///////////////////////////////////
        // Identity existence
        auto identity = identities.find(identity_id);
        // These two are already happening in the topup check, see ridl_id.cpp.
        // Uncomment them if you remove the topup check!
//        check(identity != identities.end(), "Identity does not exist");
//        check(identity->activated, "Your identity is not yet activated, please wait.");
        // END /////////////////////////////////

        ///////////////////////////////////
        // Signature verification
        checkBlockNum(block_num);

        string fragmentstring = "";
        for(auto& frag : fragments) {
            frag.assertValid();
            fragmentstring += frag.type+std::to_string(frag.value);
        }

        // Example `1:201239:reputation-1security1:1.0000 RIDL`
        string cleartext = std::to_string(identity->id) + ":" + entity + ":" + std::to_string(block_num) + ":" + fragmentstring +":"+ tokens.to_string();
        checksum256 hash = sha256(cleartext.c_str(), cleartext.size());
        assert_recover_key(hash, sig, identity->key);
        // END /////////////////////////////////


        // Basic validation
        check(tokens / fragments.size() >= (asset(1'0000, S_RIDL)/5), "You must use at least 0.2000 RIDL per reputed fragment");
        check(identity->tokens.amount >= tokens.amount, "Not enough RIDL for repute.");

        asset toPrevious = asset(tokens.amount * 0.1, S_RIDL);
        asset toSender = asset(tokens.amount * 0.05, S_RIDL);
        asset toReserves = tokens - toPrevious - toSender;

        uuid previousId = Previous(_self, _self.value).get_or_default(previous({.identity_id=0})).identity_id;
        if(previousId > 0 && previousId != identity_id) {
            // Tokens won't move accounts, just identities.
            // This allows usage tokens to flow throughout the system
            // without requiring topups, and also breaks through
            // identity expansion limits.
            auto prev = identities.find(identity_id);
            identities.modify(prev, same_payer, [&](auto& row){
                row.tokens += toPrevious;
            });
        } else toReserves += toPrevious;

        if(is_account(sender)) sendRIDL(_self, sender, toSender, "");
        else toReserves += toSender;

        sendRIDL(_self, RESERVES_ACCOUNT, toReserves, "");

        Previous(_self, _self.value).set(previous({identity_id}), _self);

        ///////////////////////////////////
        // Reducing the Identity's RIDL
        identities.modify(identity, same_payer, [&](auto& row){
            row.expansion = asset(row.expansion.amount + (tokens.amount * 0.1), S_EXP);
            row.tokens -= tokens;
        });
    }

};
