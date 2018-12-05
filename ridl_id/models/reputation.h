#pragma once

#include <eosiolib/eosio.hpp>
#include "../lib/types.h"
#include "../lib/common.h"

using namespace types;
using namespace common;
using namespace eosio;

using std::string;

namespace reputation {

    // @abi table reptypevotes
    struct RepTypeVote {
        uuid            fingerprint;
        string          type;
        uint64_t        count;

        uuid primary_key() const { return fingerprint; }
    };

    // @abi table reptypes
    struct RepTypes {
        uuid                fingerprint;
        string              type;

        uuid primary_key() const { return fingerprint; }

        ~RepTypes(){}
        RepTypes(){}
        RepTypes(uuid _fingerprint){
            fingerprint = _fingerprint;
        }
    };

    struct ReputationFragment {
        string      type;
        asset       up;
        asset       down;

        void assertValid(){
            eosio_assert(up.symbol == S_RIDL, "Reputes must be paid in RIDL");
            eosio_assert(down.symbol == S_RIDL, "Reputes must be paid in RIDL");
            eosio_assert(up.is_valid(), "Token asset is not valid");
            eosio_assert(down.is_valid(), "Token asset is not valid");
            eosio_assert(up.amount > 0 || down.amount > 0, "All reputation fragments must have either a positive or negative value");
            eosio_assert(up.amount == 0 || down.amount == 0, "Reputation fragments cannot have both positive and negative values");
        }

        bool isPositive(){
            return up.amount > 0;
        }

        bool matches( bool& otherIsUp ){
            return (isPositive() > 0   && otherIsUp)
                || (!isPositive() > 0  && !otherIsUp);
        }

        void toRepFrag(){
            up = ridlToRep(up);
            down = ridlToRep(down);
        }
    };

    // @abi table reputations
    struct Reputation {
        uuid                            fingerprint;
        vector<ReputationFragment>      fragments;
        int64_t                         total_reputes;

        ~Reputation(){}
        Reputation(){}
        Reputation(uuid _fingerprint){
            fingerprint = _fingerprint;
            total_reputes = 0;
        }

        void mergeRepute( std::vector<ReputationFragment>& frags ){
            for( auto& frag : frags ){
                string type = frag.type;
                frag.toRepFrag();

                // Frag exists
                auto found = std::find_if(fragments.begin(), fragments.end(), [type](const ReputationFragment & f) -> bool { return f.type == type; });
                if(found != fragments.end()){
                    found->up += frag.up;
                    found->down += frag.down;
                }

                // Frag does not exist
                else fragments.push_back(frag);
            }
        }
    };


    // @abi table vote
    struct Historical {
        uuid        fingerprint;
        string      data;

        uuid primary_key() const { return fingerprint; }
        EOSLIB_SERIALIZE( Historical, (fingerprint)(data) )
    };

    // @abi table fragtotal
    struct FragTotal {
        uuid        fingerprint;
        string      type;
        asset       up;
        asset       down;

        ~FragTotal(){}
        FragTotal(){}
        FragTotal(string& _type){
            lower(_type);
            fingerprint = toUUID(_type);
            type = _type;
            up = asset(0'0000, S_REP);
            down = asset(0'0000, S_REP);
        }
    };


    /***
     * Using a fake asset to build tables for
     * asset based singletons
     */
    // @abi table reptotal
    // @abi table fraghigh
    // @abi table fraglow
    struct assetmimic {
        int64_t      amount;
        symbol_code  symbol;
    };

    typedef eosio::multi_index<"typevoters"_n,     Historical>               TypeVoters;
    typedef eosio::multi_index<"reptypevotes"_n,   RepTypeVote>              ReputationTypeVotes;
    typedef eosio::multi_index<"reptypes"_n,       RepTypes>                 ReputationTypes;
    typedef eosio::singleton<"reputations"_n,      Reputation>               Reputations;
    typedef eosio::singleton<"mineowner"_n, vector<ReputationFragment>>      MineOwnerRepute;
    typedef eosio::singleton<"ownedmines"_n,vector<uuid>>                    OwnedMines;
    typedef eosio::singleton<"reptotal"_n,         asset>                    RepTotal;
    typedef eosio::singleton<"fragtotal"_n,        FragTotal>                FragTotals;

}
