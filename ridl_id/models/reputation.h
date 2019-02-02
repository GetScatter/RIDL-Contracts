#pragma once

#include <eosiolib/eosio.hpp>
#include "../lib/types.h"
#include "../lib/common.h"

using namespace types;
using namespace common;
using namespace eosio;

using std::string;

namespace reputation {



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






    struct [[eosio::table, eosio::contract("ridlridlridl")]] NewRepType {
        uuid            fingerprint;
        uuid            base;
        string          type;
        vector<name>    up;
        vector<name>    down;

        uuid primary_key() const { return fingerprint; }
    };

    struct [[eosio::table, eosio::contract("ridlridlridl")]] RepTypes {
        uuid                fingerprint;
        string              type;
        uuid                base;
        string              upTag;
        string              downTag;

        uuid primary_key() const { return fingerprint; }
        uuid by_base() const { return base; }
    };

    struct [[eosio::table, eosio::contract("ridlridlridl")]] Reputation {
        uuid                            fingerprint;
        vector<ReputationFragment>      fragments;
        int64_t                         total_reputes;

        void mergeRepute( std::vector<ReputationFragment>& frags ){
            for( auto& frag : frags ){
                string type = frag.type;
                frag.toRepFrag();

                // Frag exists
                auto found = std::find_if(
                    fragments.begin(),
                    fragments.end(),
                    [type](const ReputationFragment & f) -> bool
                    { return f.type == type; }
                );

                if(found != fragments.end()){
                    found->up += frag.up;
                    found->down += frag.down;
                }

                // Frag does not exist
                else fragments.push_back(frag);
            }
        }
    };

    struct [[eosio::table, eosio::contract("ridlridlridl")]] FragTotal {
        uuid        fingerprint;
        string      type;
        asset       up;
        asset       down;
    };





    static Reputation createReputation(uuid fingerprint){
        Reputation r;
        r.fingerprint = fingerprint;
        r.total_reputes = 0;
        return r;
    }

    static FragTotal createFragTotal(string& type){
        lower(type);
        FragTotal f;
        f.fingerprint = toUUID(type);
        f.type = type;
        f.up = asset(0'0000, S_REP);
        f.down = asset(0'0000, S_REP);
        return f;
    }


    typedef eosio::multi_index<"newreptypes"_n,    NewRepType>               NewRepTypes;
    typedef eosio::multi_index<"reptypes"_n,       RepTypes,
            indexed_by<"base"_n,    const_mem_fun<RepTypes, uint64_t, &RepTypes::by_base>>>
                                                                             ReputationTypes;


    typedef eosio::singleton<"reputations"_n,      Reputation>               Reputations;
    typedef eosio::singleton<"fragtotal"_n,        FragTotal>                FragTotals;


    // Temp fix for issue with singletons not getting proper names
    typedef eosio::multi_index<"reputations"_n,      Reputation>             Reputations_alias;
    typedef eosio::multi_index<"fragtotal"_n,        FragTotal>              FragTotals_alias;
}
