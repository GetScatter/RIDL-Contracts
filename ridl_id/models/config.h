#pragma once

using std::vector;

namespace config {

    struct [[eosio::table, eosio::contract("ridlridlridl")]] configs {
        checksum256             hash;

    };


    typedef singleton<"configs"_n, configs>   Configs;





}
