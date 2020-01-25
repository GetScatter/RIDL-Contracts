#pragma once

#include "../lib/types.h"
#include "../lib/common.h"

using namespace types;
using namespace common;

using std::string;

namespace reputation {

    struct [[eosio::table, eosio::contract("ridlridlridl")]] previous {
        uuid    identity_id;
    };

    typedef eosio::singleton<"previous"_n,      previous>               Previous;

    struct ReputationFragment {
        string      type;
        int8_t     value;

        void assertValid(){
            check(value == 1 || value == -1, "All reputation fragments must have either a positive or negative value");
        }
    };




}
