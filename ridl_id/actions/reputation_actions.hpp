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

class ReputationActions {
private:
    name _self;
    ReputationTypes reputationTypes;
    Reputables reputables;
    Identities identities;
public:
    ReputationActions(name self):_self(self),
        reputationTypes(_self, _self.value),
        reputables(_self, _self.value),
        identities(_self, _self.value){}


    void repute(string& username, string& entity, vector<ReputationFragment>& fragments){
        ///////////////////////////////////
        // Assertions and formatting
        eosio_assert(username.size() > 0, "Identity is invalid");
        eosio_assert(entity.size() > 0, "Entity is invalid");
        eosio_assert(fragments.size() <= 5 && fragments.size() > 0, "Only 1-5 reputation fragments are allowed");

        lower(username);
        lower(entity);
        uuid entityFingerprint = toUUID(entity);

        ///////////////////////////////////
        // Fragment validation
        for(auto& frag : fragments) {
            frag.assertValid();
            auto existingFrag = reputationTypes.find(frag.fingerprint);
            eosio_assert(existingFrag != reputationTypes.end(), "Fragment type is not available");
            eosio_assert(existingFrag->base == 0 || existingFrag->base == entityFingerprint, "Fragment does not belong to this entity");
            eosio_assert(existingFrag->type == frag.type, "Fingerprint does not match type name");
        }

        ///////////////////////////////////
        // Identity verification
        auto id = identities.find(toUUID(username));
        eosio_assert(id != identities.end(), "Identity does not exist");
        require_auth(id->account);

        ///////////////////////////////////
        // Calculate total RIDL used
        asset ridlUsed = asset(0'0000, S_RIDL);
        for(auto& frag : fragments) ridlUsed += (frag.up + frag.down);
        eosio_assert(id->tokens.amount >= ridlUsed.amount, "Not enough RIDL for repute.");

        ///////////////////////////////////
        // Get or create entity
        RepEntity reputable = reputable::create(entity);
        auto existing = reputables.find(reputable.fingerprint);

        ///////////////////////////////////
        // Identity reputing validation
        if(reputable.type == "id"){
            auto reputingIdentity = identities.find(entityFingerprint);
            eosio_assert(reputingIdentity != identities.end(), "Identity does not exist");

            asset totalRep = asset(0'0000, S_REP);
            for(auto& frag : fragments) totalRep += (ridlToRep(frag.up) - ridlToRep(frag.down));

            identities.modify(reputingIdentity, same_payer, [&](auto& row){
                row.total_rep += totalRep;
            });
        }

        ///////////////////////////////////
        // Add or update reputable
        if(existing == reputables.end()) createNewEntity(reputable, id, fragments);
        else updateExistingEntity(reputable, existing, id, fragments);

        ///////////////////////////////////
        // Calculating Tax and adding globals
        asset minerTax = asset(0'0000, S_RIDL);
        for(auto& frag : fragments) {
            updateFragmentTotals(frag);

            if(existing->miner != id->account){
                asset tax(((float)(frag.isPositive() ? frag.up : frag.down).amount) * 0.1, S_RIDL);
                if(reputable.hasMinerFragment(frag)) minerTax += tax;
                else minerTax -= tax;
            }
        }

        ///////////////////////////////////
        // Getting the old reputation or creating a zeroed out reputation.
        Reputation reputation = Reputations(_self, reputable.fingerprint)
            .get_or_default(reputation::createReputation(reputable.fingerprint));

        // Setting the new reputation
        reputation.mergeRepute(fragments);
        reputation.total_reputes += 1;
        Reputations(_self, reputable.fingerprint).set(reputation, reputable.miner);





//
//        /***
//         * Issuing Repute Taxes
//         */
//        asset remainingRIDL = ridlUsed;
//        asset fivePercent(((float)ridlUsed.amount * 0.05), S_RIDL);
//        string memo = "RIDL Mined - " + entity;
//
//        /***
//         * RIDL Reserves Tax
//         */
//        INLINE_ACTION_SENDER(ridl::token, transfer)( "ridlridlcoin"_n, {{_self,"active"_n}},
//                {_self, "ridlreserves"_n, fivePercent, "RIDL Tax"} );
//        remainingRIDL -= fivePercent;
//
//        /***
//         * Last Reputer Tax
//         */
//        if(existing->last_reputer && existing->last_reputer != id->account && is_account(existing->last_reputer)){
//            INLINE_ACTION_SENDER(ridl::token, transfer)( "ridlridlcoin"_n, {{_self,"active"_n}},
//                    {_self, existing->last_reputer, fivePercent, memo} );
//            remainingRIDL -= fivePercent;
//        }
//
//        /***
//         * Mine Owner
//         */
//        if(existing->miner && minerTax.amount > 0){
//            INLINE_ACTION_SENDER(ridl::token, transfer)( "ridlridlcoin"_n, {{_self,"active"_n}},
//                    {_self, existing->miner, minerTax, memo} );
//            remainingRIDL -= minerTax;
//        }
//
//        /***
//         * If this reputable has an owner then
//         * sending the remaining RIDL to the owner
//         * otherwise sending to reserves
//         */
//        if(reputable.owner)
//             INLINE_ACTION_SENDER(ridl::token, transfer)( "ridlridlcoin"_n, {{_self,"active"_n}},
//                     {_self, reputable.owner, remainingRIDL, "Reputed"} );
//        else INLINE_ACTION_SENDER(ridl::token, transfer)( "ridlridlcoin"_n, {{_self,"active"_n}},
//                {_self, "ridlreserves"_n, remainingRIDL, "Reputed unclaimed entity"} );
//
//
        // Reducing the identity's RIDL
        identities.modify(id, same_payer, [&](auto& row){ row.tokens -= ridlUsed; });
    }



    void forcetype(string& type, string& base, string& upTag, string& downTag){
        require_auth(_self);

        uuid fingerprint = toUUID(base == "" ? type : base+type);
        uuid basePrint = base == "" ? 0 : toUUID(base);

        ReputationTypes reputationTypes(_self, _self.value);
        auto iter = reputationTypes.find(fingerprint);
        if(iter != reputationTypes.end()) reputationTypes.erase(iter);
        else reputationTypes.emplace(_self, [&](auto& r){
            r.fingerprint = fingerprint;
            r.type = type;
            r.base = basePrint;
            r.upTag = upTag.size() == 0 ? "Good" : upTag;
            r.downTag = downTag.size() == 0 ? "Bad" : downTag;
        });
    }

private:



    void createNewEntity(RepEntity& reputable, Identities::const_iterator& id, vector<ReputationFragment>& fragments){
        reputable.miner = id->account;
        reputable.miner_frags = fragments;
        reputables.emplace(id->account, [&](auto& row){ row = reputable; });
    }

    void updateExistingEntity(RepEntity& reputable, Reputables::const_iterator& existing, Identities::const_iterator& id, vector<ReputationFragment>& fragments){
        reputable.merge(*existing);

        // Setting new miner
        if(!existing->miner || existing->miner_til < now()){
            reputable.miner = id->account;
            reputable.miner_til = now() + (seconds_per_day * 30);
            reputable.miner_frags = fragments;
            reputable.last_reputer = name("");
        } else {
            reputable.last_reputer = (reputable.last_reputer != id->account)
                                     ? id->account
                                     : name("");
        }

        reputables.modify(existing, same_payer, [&](auto& row){ row = reputable; });
    }

    void updateFragmentTotals( ReputationFragment& frag ){
        FragTotal fragTotal = FragTotals(_self, frag.fingerprint).get_or_default(reputation::createFragTotal(frag.type));
        fragTotal.up += ridlToRep(frag.up);
        fragTotal.down += ridlToRep(frag.down);
        FragTotals(_self, frag.fingerprint).set(fragTotal, _self);
    }


};
