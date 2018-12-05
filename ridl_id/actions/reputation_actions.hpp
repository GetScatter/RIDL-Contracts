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
    name __self;
    ReputationActions(name self):__self(self){}


    void repute(string& username, string& entity, vector<ReputationFragment>& fragments){

        /***
         * Validating & Formatting Parameters
         */
        eosio_assert(username.size() > 0, "Identity is invalid");
        eosio_assert(entity.size() > 0, "Entity is invalid");
        eosio_assert(fragments.size() <= 8 && fragments.size() > 0, "Only 1-8 reputation fragments are allowed");
        for(auto& frag : fragments) frag.assertValid();
        lower(username);
        lower(entity);
        uuid fingerprint = toUUID(entity);

        /***
         * Validating and proving reputer
         */
        Identities identities(__self, __self);
        auto id = identities.find(toUUID(username));
        eosio_assert(id != identities.end(), "Identity does not exist");
        require_auth(id->account);


        /***
         * Creating / Updating Reputable Entity
         */
        RepEntity reputable(entity);
        Reputables reputables(__self, __self);
        auto existing = reputables.find(fingerprint);
        std::map<string, bool> minerFragments;

        // Reputable already has a mine owner
        if(existing != reputables.end() && existing->miner_til > now()) {
            // Merging existing entity into entity
            reputable.merge(*existing);

            // Setting next last_reputer
            reputable.last_reputer = (reputable.last_reputer != id->account)
                                     ? id->account
                                     : 0;

            // Updating Row
            reputables.modify(existing, 0, [&](auto& record){ record = reputable; });

            // Setting up miner fragments for payout
            vector<ReputationFragment> frags = MineOwnerRepute(__self, fingerprint).get();
            for(auto& frag : frags) minerFragments[frag.type] = frag.isPositive();
        }

        // Reputable is a new entity
        else {
            if(reputable.miner){
                // Removing previous owner from their own owned mines
                vector<uuid> owned = OwnedMines(__self, reputable.miner).get_or_default(vector<uuid>());
                owned.erase(std::remove(owned.begin(), owned.end(), fingerprint), owned.end());
                OwnedMines(__self, reputable.miner).set(owned, reputable.miner);
            }


            // Setting first or next reputer as owner
            reputable.miner = id->account;

            // Setting row
            reputables.emplace(id->account, [&](auto& record){ record = reputable; });

            // Setting up miner fragments for future payouts
            MineOwnerRepute(__self, fingerprint).set(fragments, id->account);




            // Adding mine to owner their owned mines
            vector<uuid> owned = OwnedMines(__self, id->account).get_or_default(vector<uuid>());
            owned.push_back(fingerprint);
            OwnedMines(__self, id->account).set(owned, id->account);
        }


        /***
         * Getting the old reputation or creating a zeroed out reputation.
         */
        Reputation reputation = Reputations(__self, fingerprint).get_or_default(Reputation(fingerprint));


        /***
         * Getting the old REP total for this entity, and it's payer.
         */
        asset repTotal = RepTotal(__self, fingerprint).get_or_default(asset(0'0000, string_to_symbol(4, "REP")));
        name repTotalPayer = RepTotal(__self, fingerprint).exists() ? 0 : id->account;


        /***
         * Calculating Mine Owner Tax and Total RIDL Used
         */
        asset minerTax = asset(0'0000, string_to_symbol(4, "RIDL"));
        asset ridlUsed = asset(0'0000, string_to_symbol(4, "RIDL"));


        ReputationTypes reputationTypes(__self, __self);

        for(auto& frag : fragments){
            eosio_assert(reputationTypes.find(toUUID(frag.type)) != reputationTypes.end(), "One of the fragment types does not exist");

            repTotal += ridlToRep(frag.up) - ridlToRep(frag.down);
            ridlUsed += frag.up + frag.down;

            /***
             * Updating the global fragment totals
             */
            updateFragmentTotals(frag);

            /***
             * Calculating miner tax from fragments
             */
            auto it = minerFragments.find(frag.type);
            if(it != minerFragments.end()){
                asset res(((float)(frag.isPositive() ? frag.up : frag.down).amount) * 0.1, string_to_symbol(4, "RIDL"));
                if((frag.matches(it->second))) minerTax += res;
                else minerTax -= res;
            }
        }

        eosio_assert(ridlUsed <= id->tokens, "Not enough available RIDL on this Identity for this Repute.");


        /***
         * Setting the new reputation
         */
        reputation.mergeRepute(fragments);
        reputation.total_reputes += 1;
        Reputations(__self, fingerprint).set(reputation, reputable.miner);



        /***
         * Updating the total rep across all fragments for this entity
         */
        RepTotal(__self, fingerprint).set(repTotal, repTotalPayer);


        /***
         * Issuing Repute Taxes
         */
        asset remainingRIDL = ridlUsed;
        asset fivePercent(((float)ridlUsed.amount * 0.05), string_to_symbol(4, "RIDL"));
        string memo = "RIDL Mined - " + entity;

        /***
         * RIDL Reserves Tax
         */
        INLINE_ACTION_SENDER(ridl::token, transfer)( N(ridlridlcoin), {{__self,N(active)}}, {__self, N(ridlreserves), fivePercent, "RIDL Tax"} );
        remainingRIDL -= fivePercent;

        /***
         * Last Reputer Tax
         */
        if(existing->last_reputer && existing->last_reputer != id->account && is_account(existing->last_reputer)){
            INLINE_ACTION_SENDER(ridl::token, transfer)( N(ridlridlcoin), {{__self,N(active)}}, {__self, existing->last_reputer, fivePercent, memo} );
            remainingRIDL -= fivePercent;
        }

        /***
         * Mine Owner
         */
        if(existing->miner && minerTax.amount > 0){
            INLINE_ACTION_SENDER(ridl::token, transfer)( N(ridlridlcoin), {{__self,N(active)}}, {__self, existing->miner, minerTax, memo} );
            remainingRIDL -= minerTax;
        }

        /***
         * If this reputable has an owner then
         * sending the remaining RIDL to the owner
         * otherwise sending to reserves
         */
        if(reputable.owner)
             INLINE_ACTION_SENDER(ridl::token, transfer)( N(ridlridlcoin), {{__self,N(active)}}, {__self, reputable.owner, remainingRIDL, "Reputed"} );
        else INLINE_ACTION_SENDER(ridl::token, transfer)( N(ridlridlcoin), {{__self,N(active)}}, {__self, N(ridlreserves), remainingRIDL, "Reputed unclaimed entity"} );


        // Reducing the identity's RIDL
        identities.modify(id, 0, [&](auto& record){ record.tokens -= ridlUsed; });
    }



    void unrepute( string& username, string& entity ){
        eosio_assert(username.size() > 0, "Identity is invalid");
        eosio_assert(entity.size() > 0, "Entity is invalid");

        lower(username);
        lower(entity);
        uuid fingerprint = toUUID(entity);

        Identities identities(__self, __self);
        auto id = identities.find(toUUID(username));
        eosio_assert(id != identities.end(), "Identity does not exist");
        require_auth(id->account);

        RepEntity reputable(entity);
        Reputables reputables(__self, __self);
        auto existing = reputables.find(fingerprint);

        eosio_assert(existing != reputables.end(), "This reputable entity does not exist");
        eosio_assert(RepTotal(__self, fingerprint).exists(), "This entity does not have any REP");

        asset minerRepTotal(0'0000, string_to_symbol(4, "REP"));
        asset repTotal = RepTotal(__self, fingerprint).get();
        vector<ReputationFragment> frags = MineOwnerRepute(__self, fingerprint).get();

        for(auto& frag : frags) minerRepTotal += ridlToRep(frag.up) - ridlToRep(frag.down);
        print("Miner rep total: ", minerRepTotal, " | ");
        print("Rep total: ", repTotal, " | ");
        eosio_assert(minerRepTotal.amount == repTotal.amount, "Others have already reputed this entity");

        vector<uuid> owned = OwnedMines(__self, id->account).get_or_default(vector<uuid>());
        owned.erase(std::remove(owned.begin(), owned.end(), fingerprint), owned.end());
        OwnedMines(__self, id->account).set(owned, id->account);

        reputables.erase(existing);
        MineOwnerRepute(__self, fingerprint).remove();
        RepTotal(__self, fingerprint).remove();

        for(auto& frag : frags){
            uuid fragTypeFingerprint = toUUID(frag.type);
            if(FragTotals(__self, fragTypeFingerprint).exists()){
                FragTotal fragTotal = FragTotals(__self, fragTypeFingerprint).get_or_default(FragTotal(frag.type));
                fragTotal.up -= ridlToRep(frag.up);
                fragTotal.down -= ridlToRep(frag.down);
                FragTotals(__self, fragTypeFingerprint).set(fragTotal, __self);
            }
        }
    }


    void votetype( string& username, string& type ){
        uuid fingerprint = toUUID(type);
        uuid idprint = toUUID(username);

        string idtype = username + type;
        uuid idtypeprint = toUUID(idtype);

        Identities identities(__self, __self);
        auto id = identities.find(idprint);
        eosio_assert(id != identities.end(), "Identity does not exist");
        require_auth(id->account);

        asset userRep = RepTotal(__self, idprint).get_or_default(asset(0'0000, string_to_symbol(4, "REP")));
        eosio_assert(userRep.amount > 1000, "Type voters must have 1000+ total REP");

        ReputationTypes reputationTypes(__self, __self);
        auto existing = reputationTypes.find(fingerprint);
        eosio_assert(existing == reputationTypes.end(), "Type already exists in available types");

        ReputationTypeVotes reputationTypeVotes(__self, __self);
        auto typeVote = reputationTypeVotes.find(fingerprint);

        TypeVoters typeVoters(__self, idtypeprint);

        if(typeVote == reputationTypeVotes.end()) reputationTypeVotes.emplace(id->account, [&](auto& record){
            record.fingerprint = fingerprint;
            record.type = type;
            record.count = 1;
            typeVoters.emplace(id->account, [&](auto& r){ r.fingerprint = idprint; });
        });
        else {
            if(typeVote->count+1 >= 20){
                // Erasing all voters
                while(typeVoters.begin() != typeVoters.end()){
                    auto iter = --typeVoters.end();
                    typeVoters.erase(iter);
                }

                // Adding new type
                reputationTypes.emplace(__self, [&](auto& r){
                    r.fingerprint = fingerprint;
                    r.type = type;
                });

                // Removing vote-able type
                reputationTypeVotes.erase(typeVote);
            }

            // Not enough votes yet, incrementing
            else reputationTypeVotes.modify(typeVote, 0, [&](auto& record){
                auto voted = typeVoters.find(idprint);
                if(voted != typeVoters.end()){
                    record.count -= 1;
                    typeVoters.erase(voted);
                } else {
                    record.count += 1;
                    typeVoters.emplace(id->account, [&](auto& r){ r.fingerprint = idprint; });
                }
            });
        }
    }

    void forcetype(string& type){
        require_auth(__self);

        uuid fingerprint = toUUID(type);

        ReputationTypes reputationTypes(__self, __self);
        auto iter = reputationTypes.find(fingerprint);
        if(iter != reputationTypes.end()) reputationTypes.erase(iter);
        else reputationTypes.emplace(__self, [&](auto& r){
            r.fingerprint = fingerprint;
            r.type = type;
        });
    }

private:

    void updateFragmentTotals( ReputationFragment& frag ){
        uuid fingerprint = toUUID(frag.type);

        FragTotal fragTotal = FragTotals(__self, fingerprint).get_or_default(FragTotal(frag.type));
        fragTotal.up += ridlToRep(frag.up);
        fragTotal.down += ridlToRep(frag.down);
        FragTotals(__self, fingerprint).set(fragTotal, __self);
    }


};
