#pragma once

namespace config {

    struct [[eosio::table, eosio::contract("ridlridlridl")]] configs {
        name    identity_creator;
    };

    typedef eosio::singleton<"config"_n,      configs>               Configs;
}
