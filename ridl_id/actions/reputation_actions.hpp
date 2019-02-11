#pragma once

#include <eosiolib/eosio.hpp>
#include "../models/reputable.h"
#include "../models/reputation.h"
#include "../models/identity.h"
#include "../../ridl_token/ridl_token.hpp"

using namespace eosio;
using namespace reputable;
using namespace reputation;
using namespace identity;

using std::string;
using std::vector;
using std::make_tuple;

class ReputationActions {
private:
    name _self;
    ReputationTypes reputationTypes;
    Reputables reputables;
    Identities identities;

    auto findIdentity(string& username){
        lower(username);
        auto index = identities.get_index<"name"_n>();
        auto found = index.find(toUUID(username));
        eosio_assert(found != index.end(), ("Could not find username: "+username).c_str());
        return identities.find(found->id);
    }

public:
    ReputationActions(name self):_self(self),
        reputationTypes(_self, _self.value),
        reputables(_self, _self.value),
        identities(_self, _self.value){}

    void repute(string& username, uuid id, string& entity, string& type, vector<ReputationFragment>& fragments, string& network, uuid& parent){
        ///////////////////////////////////
        // Assertions and formatting
        eosio_assert(entity.size() > 0, "Entity is invalid");
        eosio_assert(fragments.size() <= 5 && fragments.size() > 0, "Only 1-5 reputation fragments are allowed");

        lower(entity);
        lower(network);

        ///////////////////////////////////
        // Get or create entity
        bool isNew = id == 0;

        // Checks if an entity with the same fingerprint exists and
        // just the ID wasn't sent
        if(isNew) {
            uint64_t fingerprint = toUUID(type+entity+network+std::to_string(parent));
            auto index = reputables.get_index<"fingerprint"_n>();
            auto existing = index.find(fingerprint);
            if(existing != index.end()){
                id = existing->id;
                isNew = false;
            }
        }

        Reputable reputable = reputable::create(entity, type, network, parent);
        auto existingReputable = reputables.find(id);

        if(!isNew) eosio_assert(existingReputable != reputables.end(), ("There is no reputable with the ID "+std::to_string(id)).c_str());

        ///////////////////////////////////
        // Fragment validation
        for(auto& frag : fragments) {
            frag.assertValid();
            auto existingFrag = reputationTypes.find(frag.fingerprint);
            eosio_assert(existingFrag != reputationTypes.end(), "Fragment type is not available");
            eosio_assert(existingFrag->base == 0 || existingFrag->base == id, "Fragment does not belong to this entity");
            eosio_assert(existingFrag->type == frag.type, "Fingerprint does not match type name");
        }

        ///////////////////////////////////
        // Identity verification
        auto identity = findIdentity(username);
        eosio_assert(identity != identities.end(), "Identity does not exist");
        identity->authenticate();

        ///////////////////////////////////
        // Calculate total RIDL used
        asset ridlUsed = asset(0'0000, S_RIDL);
        for(auto& frag : fragments) ridlUsed += (frag.up + frag.down);
        eosio_assert(identity->tokens.amount >= ridlUsed.amount, "Not enough RIDL for repute.");

        // If a base is provided, then
        if(parent > 0){
            // Base existence validation
            auto baseReputable = reputables.find(parent);
            eosio_assert(baseReputable != reputables.end(), "There is no such base reputable.");

            ///////////////////////////////////
            // Action reputing validation
            if(reputable.type == "act"){
                eosio_assert(baseReputable->type == "acc", "The base reputable of an action must be a blockchain accounts/contract.");
            }
        }

        ///////////////////////////////////
        // Add or update reputable
        if(isNew) createNewEntity(reputable, identity, fragments);
        else updateExistingEntity(reputable, existingReputable, identity, fragments);

        ///////////////////////////////////
        // Identity reputing validation
        if(reputable.type == "id") reputedIdentity(reputable.id, fragments);

        ///////////////////////////////////
        // Calculating Tax and adding globals
        asset minerTax = asset(0'0000, S_RIDL);
        for(auto& frag : fragments) {
            updateFragmentTotals(frag);

            if(existingReputable->miner != identity->account){
                asset tax(((float)(frag.isPositive() ? frag.up : frag.down).amount) * 0.1, S_RIDL);
                if(reputable.hasMinerFragment(frag)) minerTax += tax;
                else minerTax -= tax;
            }
        }

        ///////////////////////////////////
        // Getting the old reputation or creating a zeroed out reputation.
        Reputation reputation = Reputations(_self, reputable.id)
            .get_or_default(reputation::createReputation(reputable.id));

        ///////////////////////////////////
        // Setting the new reputation
        reputation.mergeRepute(fragments);
        reputation.total_reputes += 1;
        Reputations(_self, reputable.id).set(reputation, reputable.miner);



        ///////////////////////////////////
        // Issuing Repute Taxes
        asset remainingRIDL = ridlUsed;
        asset fivePercent(((float)ridlUsed.amount * 0.05), S_RIDL);
        string memo = "RIDL Mined - " + entity;

        ///////////////////////////////////
        // RIDL Reserves Tax
        sendRIDL(_self, "ridlreserves"_n, fivePercent, "RIDL Tax");
        remainingRIDL -= fivePercent;

        ///////////////////////////////////
        // Last reputer mined
        if(existingReputable->last_reputer && existingReputable->last_reputer != identity->account && is_account(existingReputable->last_reputer)){
            sendRIDL(_self, existingReputable->last_reputer, fivePercent, memo);
            remainingRIDL -= fivePercent;
        }

        ///////////////////////////////////
        // Mine owner mined
        if(existingReputable->miner && minerTax.amount > 0){
            sendRIDL(_self, existingReputable->miner, minerTax, memo);
            remainingRIDL -= minerTax;
        }

        /***
         * If this reputable has an owner then
         * sending the remaining RIDL to the owner
         * otherwise sending to reserves
         */
        if(reputable.owner) sendRIDL(_self, reputable.owner, remainingRIDL, "Reputed");
        else sendRIDL(_self, "ridlreserves"_n, remainingRIDL, "Reputed unclaimed entity");

        ///////////////////////////////////
        // Reducing the Identity's RIDL
        identities.modify(identity, same_payer, [&](auto& row){
            row.expansion = asset(row.expansion.amount + (ridlUsed.amount * 0.01), S_EXP);
            row.tokens -= ridlUsed;
        });
    }



    void forcetype(string& type, uuid& parent, string& upTag, string& downTag){
        require_auth(_self);

        eosio_assert(parent == 0 || reputables.find(parent) != reputables.end(), ("There is no reputable with the ID: "+std::to_string(parent)).c_str());

        RepType repType = RepType::create(type, upTag, downTag, parent);

        ReputationTypes reputationTypes(_self, _self.value);
        auto iter = reputationTypes.find(repType.fingerprint);
        eosio_assert(iter == reputationTypes.end(), "This rep type already exists.");

        reputationTypes.emplace(_self, [&](auto& r){ r = repType; });
    }

private:



    void createNewEntity(Reputable& reputable, Identities::const_iterator& identity, vector<ReputationFragment>& fragments){
        reputable.miner = identity->account;
        reputable.miner_frags = fragments;
        reputable.last_repute_time = now();
        reputable.id = reputables.available_primary_key();
        if(reputable.id == 0) reputable.id = 1;
        reputables.emplace(identity->account, [&](auto& row){ row = reputable; });
    }

    void updateExistingEntity(Reputable& reputable, Reputables::const_iterator& existingReputable, Identities::const_iterator& identity, vector<ReputationFragment>& fragments){
        reputable.merge(*existingReputable);

        // Setting new miner
        if(!existingReputable->miner || existingReputable->miner_til < now()){
            reputable.miner = identity->account;
            reputable.miner_til = now() + (SECONDS_PER_DAY * 30);
            reputable.miner_frags = fragments;
            reputable.last_reputer = name("");
        } else {
            reputable.last_reputer = (reputable.last_reputer != identity->account)
                                     ? identity->account
                                     : name("");
        }

        reputable.last_repute_time = now();
        reputables.modify(existingReputable, same_payer, [&](auto& row){ row = reputable; });
    }

    void reputedIdentity(uuid& id, vector<ReputationFragment>& fragments){
        auto reputingIdentity = identities.find(id);
        eosio_assert(reputingIdentity != identities.end(), "Identity does not exist");

        asset totalRep = asset(0'0000, S_REP);
        for(auto& frag : fragments) totalRep += (ridlToRep(frag.up) - ridlToRep(frag.down));

        identities.modify(reputingIdentity, same_payer, [&](auto& row){
            row.total_rep += totalRep;
        });
    }

    void updateFragmentTotals( ReputationFragment& frag ){
        FragTotal fragTotal = FragTotals(_self, frag.fingerprint).get_or_default(reputation::createFragTotal(frag.type));
        fragTotal.up += ridlToRep(frag.up);
        fragTotal.down += ridlToRep(frag.down);
        FragTotals(_self, frag.fingerprint).set(fragTotal, _self);
    }


};
