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
        EOSLIB_SERIALIZE( RepTypeVote, (fingerprint)(type)(count) )
    };

    // @abi table reptypes
    struct RepTypes {
        uuid                fingerprint;
        string              type;

        uuid primary_key() const { return fingerprint; }
        EOSLIB_SERIALIZE( RepTypes, (fingerprint)(type) )

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
            eosio_assert(up.symbol == string_to_symbol(4, "RIDL"), "Reputes must be paid in RIDL");
            eosio_assert(down.symbol == string_to_symbol(4, "RIDL"), "Reputes must be paid in RIDL");
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

        EOSLIB_SERIALIZE( Reputation, (fingerprint)(fragments)(total_reputes) )

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
            up = asset(0'0000, string_to_symbol(4, "REP"));
            down = asset(0'0000, string_to_symbol(4, "REP"));
        }

        EOSLIB_SERIALIZE( FragTotal, (fingerprint)(type)(up)(down) )
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
        symbol_type  symbol;
    };

    typedef eosio::multi_index<N(typevoters),     Historical>               TypeVoters;
    typedef eosio::multi_index<N(reptypevotes),   RepTypeVote>              ReputationTypeVotes;
    typedef eosio::multi_index<N(reptypes),       RepTypes>                 ReputationTypes;
    typedef eosio::singleton<N(reputations),      Reputation>               Reputations;
    typedef eosio::singleton<N(mineowner), vector<ReputationFragment>>      MineOwnerRepute;
    typedef eosio::singleton<N(ownedmines),vector<uuid>>                    OwnedMines;
    typedef eosio::singleton<N(reptotal),         asset>                    RepTotal;
    typedef eosio::singleton<N(fragtotal),        FragTotal>                FragTotals;

}
