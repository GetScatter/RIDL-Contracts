#pragma once

#include "../lib/types.h"
#include "../lib/common.h"
using namespace types;
using namespace common;

namespace paid {

    struct Paid {
        uuid            fingerprint;
        name            account;
        name            payer;
        uint64_t        expires;

        Paid(){}
        Paid(uuid f, name a, name p)
                :fingerprint(f),
                 account(a),
                 payer(p),
                 expires(now() + (seconds_per_day * 365)){}

        uuid primary_key() const { return fingerprint; }
    };

    typedef eosio::multi_index<"paid"_n, Paid> PaidNames;
}
