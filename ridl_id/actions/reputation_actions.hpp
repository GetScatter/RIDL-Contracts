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

    void repute(string& username, uuid id, string& entity, string& type, vector<ReputationFragment>& fragments, string& network, string& parentString){
        ///////////////////////////////////
        // Assertions and formatting
        eosio_assert(entity.size() > 0, "Entity is invalid");
        eosio_assert(fragments.size() <= 5 && fragments.size() > 0, "Only 1-5 reputation fragments are allowed");

        lower(entity);
        lower(network);


        ///////////////////////////////////
        // Identity verification
        auto identity = findIdentity(username);
        eosio_assert(identity != identities.end(), "Identity does not exist");
        identity->authenticate();

        ///////////////////////////////////
        // Parent handling and parsing
        uuid parent = getReputableParentOrCreate(parentString, identity);

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

        Reputable reputable = Reputable::create(entity, type, network, parent);
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
        // Calculate total RIDL used
        asset ridlUsed = asset(0'0000, S_RIDL);
        for(auto& frag : fragments) ridlUsed += (frag.up + frag.down);
        eosio_assert(identity->tokens.amount >= ridlUsed.amount, "Not enough RIDL for repute.");


        // Contract actions must have a parent
        if(reputable.type == "act"){
            eosio_assert(parent != 0, "Contract actions must be parented to a contract.");
        }

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
        if(reputable.type == "id") {
            reputedIdentity(reputable, fragments);
        }

        ///////////////////////////////////
        // Getting the old reputation or creating a zeroed out reputation.
        Reputation reputation = Reputations(_self, reputable.id)
            .get_or_default(reputation::createReputation(reputable.id));

        ///////////////////////////////////
        // Setting the new reputation
        reputation.mergeRepute(fragments);
        reputation.total_reputes += 1;
        Reputations(_self, reputable.id).set(reputation, _self);


        ///////////////////////////////////
        // Pay reputation taxes
        payTaxes(ridlUsed, reputable, identity, fragments, existingReputable->last_reputer);

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

    /***
     * Creates a parent or returns an existing one.
     * @param parentString
     * @param identity
     * @return
     */
    uuid getReputableParentOrCreate(string& parentString, Identities::const_iterator& identity){
        if(parentString.find("::", 0) == std::string::npos) return 0;

        vector<string> parts = splitString(parentString, "::");
        if(parts[0] == "id") return std::stoull(parts[1]);

        eosio_assert(parts[0] == "fingerprint", "Parent must be either an ID or a Fingerprint.");

        string& entity = parts[1];
        string& type = parts[2];
        string network = parts.size() > 3 ? splitString(parentString, parts[2]+"::")[1] : "";

        lower(entity);
        lower(type);
        lower(network);

        uuid p = 0;
        Reputable reputable = Reputable::create(entity, type, network, p);

        // Looking for existing parent based on fingerprinting
        auto index = reputables.get_index<"fingerprint"_n>();
        auto existing = index.find(reputable.by_fingerprint());
        if(existing != index.end()) return existing->id;

        // Creating new parent
        vector<ReputationFragment> fragments;
        createNewEntity(reputable, identity, fragments);
        return reputable.id;
    }

    /***
     * Updates or creates miner fragment references.
     * @param reputable
     * @param identity
     * @param fragments
     */
    void updateOrCreateMinerFrags(Reputable& reputable, Identities::const_iterator& identity, vector<ReputationFragment>& fragments){
        MinerFrags minerFrags(_self, reputable.id);
        for(auto& frag : fragments) {
            auto minerFrag = minerFrags.find(frag.fingerprint);
            if(minerFrag != minerFrags.end()){
                if(minerFrag->expired()) minerFrags.modify(minerFrag, identity->account, [&](auto& row){
                    row = MinerFrag::create(identity, frag);
                });
            }
            else minerFrags.emplace(identity->account, [&](auto& row){
                row = MinerFrag::create(identity, frag);
            });
        }
    }

    void createNewEntity(Reputable& reputable, Identities::const_iterator& identity, vector<ReputationFragment>& fragments){
        reputable.last_repute_time = now();
        reputable.id = reputables.available_primary_key();
        if(reputable.id == 0) reputable.id = 1;
        reputable.block = tapos_block_num();
        reputables.emplace(identity->account, [&](auto& row){ row = reputable; });

        updateOrCreateMinerFrags(reputable, identity, fragments);
    }

    void updateExistingEntity(Reputable& reputable, Reputables::const_iterator& existingReputable, Identities::const_iterator& identity, vector<ReputationFragment>& fragments){
        reputable.merge(*existingReputable);

        updateOrCreateMinerFrags(reputable, identity, fragments);

        reputable.last_reputer = reputable.last_reputer != identity->account
                                 ? identity->account
                                 : name("");

        reputable.last_repute_time = now();
        reputables.modify(existingReputable, same_payer, [&](auto& row){ row = reputable; });
    }

    void reputedIdentity(Reputable& reputable, vector<ReputationFragment>& fragments){
        auto reputingIdentity = findIdentity(reputable.entity);
        eosio_assert(reputingIdentity != identities.end(), "Identity does not exist");

        asset totalRep = asset(0'0000, S_REP);
        for(auto& frag : fragments) totalRep += (ridlToRep(frag.up) - ridlToRep(frag.down));

        identities.modify(reputingIdentity, same_payer, [&](auto& row){
            row.total_rep    += totalRep;
            row.usable_rep   += asset(totalRep.amount * 0.5, S_REP);
        });
    }

    void updateFragmentTotals( ReputationFragment& frag ){
        FragTotal fragTotal = FragTotals(_self, frag.fingerprint).get_or_default(reputation::createFragTotal(frag.type));
        fragTotal.up += ridlToRep(frag.up);
        fragTotal.down += ridlToRep(frag.down);
        FragTotals(_self, frag.fingerprint).set(fragTotal, _self);
    }

    void payTaxes(asset ridlUsed, Reputable& reputable, Identities::const_iterator& identity, vector<ReputationFragment>& fragments, name last_reputer){



        ///////////////////////////////////
        // Issuing Repute Taxes
        asset remainingRIDL = ridlUsed;
        asset fivePercent(((float)ridlUsed.amount * 0.05), S_RIDL);



        ///////////////////////////////////
        // Calculating Tax and adding globals
        MinerFrags minerFrags(_self, reputable.id);
        asset minerTax = asset(0'0000, S_RIDL);

        std::map<name, asset> minerTaxes = std::map<name, asset>();

        for(auto& frag : fragments) {
            updateFragmentTotals(frag);

            MinerFrag minerFrag = minerFrags.get(frag.fingerprint, "Could not get miner frag");
            Identity minerId = identities.get(minerFrag.identity, "Identity no longer exists");

            if(minerFrag.identity != identity->id){
                asset tax(((float)(frag.isPositive() ? frag.up : frag.down).amount) * 0.1, S_RIDL);

                if(minerTaxes.find(minerId.account) == minerTaxes.end()) {
                    minerTaxes[minerId.account] = asset(0'0000, S_RIDL);
                }

                if(minerFrag.positive == frag.isPositive()) minerTaxes[minerId.account] += tax;
                else minerTaxes[minerId.account] -= tax;
            }
        }


        for(const auto& pair : minerTaxes){
            if(pair.second.amount > 0) {
                sendRIDL(_self, pair.first, pair.second, (pair.second.to_string() + " Mined - " + reputable.entity));
                remainingRIDL -= minerTax;
            }
        }




        ///////////////////////////////////
        // RIDL Reserves Tax
        sendRIDL(_self, "ridlreserves"_n, fivePercent, "RIDL Tax");
        remainingRIDL -= fivePercent;

        ///////////////////////////////////
        // Last reputer mined
        if(last_reputer && last_reputer != identity->account && is_account(last_reputer)){
            sendRIDL(_self, last_reputer, fivePercent, (fivePercent.to_string() + " Mined - " + reputable.entity));
            remainingRIDL -= fivePercent;
        }

        /***
         * If this reputable has an owner then
         * sending the remaining RIDL to the owner
         * otherwise sending to reserves
         */
        if(reputable.owner) sendRIDL(_self, reputable.owner, remainingRIDL, "Reputed");
        else sendRIDL(_self, "ridlreserves"_n, remainingRIDL, "Reputed unclaimed entity");
    }


};
