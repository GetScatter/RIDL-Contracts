#pragma once

namespace balances {

    struct [[eosio::table, eosio::contract("ridlridlridl")]] balances {
        uint64_t symbol;
        asset balance;

        uint64_t primary_key() const { return symbol; }
    };

    typedef eosio::multi_index<"balances"_n, balances> Balances;
}
