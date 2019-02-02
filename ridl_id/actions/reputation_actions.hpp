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
public:
    name _self;
    ReputationActions(name self):_self(self){}


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
        ReputationTypes reputationTypes(_self, _self.value);
        for(auto& frag : fragments) {
            frag.assertValid();
            bool isGlobal = reputationTypes.find(toUUID(frag.type)) != reputationTypes.end();
            bool isBased =  reputationTypes.find(toUUID(entity+frag.type)) != reputationTypes.end();
            eosio_assert(isGlobal || isBased, "Fragment type is not available");
        }

        ///////////////////////////////////
        // Identity verification
        Identities identities(_self, _self.value);
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
        Reputables reputables(_self, _self.value);
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
        // New reputable
        if(existing == reputables.end()){
            reputable.miner = id->account;
            reputable.miner_frags = fragments;
            reputables.emplace(id->account, [&](auto& row){ row = reputable; });
        }

        ///////////////////////////////////
        // Existing reputable
        else {
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
//        // Reducing the identity's RIDL
        identities.modify(id, same_payer, [&](auto& row){ row.tokens -= ridlUsed; });
    }

//
//
//    void votetype( string& username, string& type ){
//        uuid fingerprint = toUUID(type);
//        uuid idprint = toUUID(username);
//
//        string idtype = username + type;
//        uuid idtypeprint = toUUID(idtype);
//
//        Identities identities(_self, _self.value);
//        auto id = identities.find(idprint);
//        eosio_assert(id != identities.end(), "Identity does not exist");
//        require_auth(id->account);
//
//        asset userRep = RepTotal(_self, idprint.value).get_or_default(asset(0'0000, S_REP));
//        eosio_assert(userRep.amount > 1000, "Type voters must have 1000+ total REP");
//
//        ReputationTypes reputationTypes(_self, _self.value);
//        auto existing = reputationTypes.find(fingerprint);
//        eosio_assert(existing == reputationTypes.end(), "Type already exists in available types");
//
//        ReputationTypeVotes reputationTypeVotes(_self, _self.value);
//        auto typeVote = reputationTypeVotes.find(fingerprint);
//
//        TypeVoters typeVoters(_self, idtypeprint);
//
//        if(typeVote == reputationTypeVotes.end()) reputationTypeVotes.emplace(id->account, [&](auto& row){
//            row.fingerprint = fingerprint;
//            row.type = type;
//            row.count = 1;
//            typeVoters.emplace(id->account, [&](auto& r){ r.fingerprint = idprint; });
//        });
//        else {
//            if(typeVote->count+1 >= 20){
//                // Erasing all voters
//                while(typeVoters.begin() != typeVoters.end()){
//                    auto iter = --typeVoters.end();
//                    typeVoters.erase(iter);
//                }
//
//                // Adding new type
//                reputationTypes.emplace(_self, [&](auto& r){
//                    r.fingerprint = fingerprint;
//                    r.type = type;
//                });
//
//                // Removing vote-able type
//                reputationTypeVotes.erase(typeVote);
//            }
//
//            // Not enough votes yet, incrementing
//            else reputationTypeVotes.modify(typeVote, same_payer, [&](auto& row){
//                auto voted = typeVoters.find(idprint);
//                if(voted != typeVoters.end()){
//                    row.count -= 1;
//                    typeVoters.erase(voted);
//                } else {
//                    row.count += 1;
//                    typeVoters.emplace(id->account, [&](auto& r){ r.fingerprint = idprint; });
//                }
//            });
//        }
//    }
//
    void forcetype(string& type, string& base, string& up, string& down){
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
            r.upTag = up.size() == 0 ? "Good" : up;
            r.downTag = down.size() == 0 ? "Bad" : down;
        });
    }

private:

    void updateFragmentTotals( ReputationFragment& frag ){
        uuid fingerprint = toUUID(frag.type);

        FragTotal fragTotal = FragTotals(_self, fingerprint).get_or_default(reputation::createFragTotal(frag.type));
        fragTotal.up += ridlToRep(frag.up);
        fragTotal.down += ridlToRep(frag.down);
        FragTotals(_self, fingerprint).set(fragTotal, _self);
    }


};
