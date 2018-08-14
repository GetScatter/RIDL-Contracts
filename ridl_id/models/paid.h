#pragma once

#include "../lib/types.h"
#include "../lib/common.h"
using namespace types;
using namespace common;

namespace paid {
    // @abi table paid
    struct Paid {
        uuid            fingerprint;
        account_name    account;
        account_name    payer;
        uint64_t        expires;

        Paid(){}
        Paid(uuid f, account_name a, account_name p)
                :fingerprint(f),
                 account(a),
                 payer(p),
                 expires(now() + (seconds_per_day * 365)){}

        account_name primary_key() const { return fingerprint; }
        EOSLIB_SERIALIZE( Paid, (fingerprint)(account)(payer)(expires) )
    };

    typedef eosio::multi_index<N(paid), Paid> PaidNames;
}
