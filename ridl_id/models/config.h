#pragma once

namespace config {

    struct [[eosio::table, eosio::contract("ridlridlridl")]] configs {
        name        manager;
        uint64_t    starting_block;
    };

    typedef eosio::singleton<"configs"_n,      configs>               Configs;
}
