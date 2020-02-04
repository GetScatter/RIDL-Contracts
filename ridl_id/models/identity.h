#pragma once

#include "../lib/types.h"
#include "../lib/common.h"
using namespace types;
using namespace common;

using std::vector;
using std::string;

namespace identity {

    struct [[eosio::table, eosio::contract("ridlridlridl")]] Identity {
        uuid                    id;
        uuid                    fingerprint;
        string                  username;
        public_key              key;
        asset                   tokens;
        asset                   expansion;
        uint64_t                created;
        uint64_t                block;
        uint8_t                 activated;

        uuid primary_key() const { return id; }
        uint64_t by_name() const {return fingerprint; }


        static Identity create(string& username, const public_key& key){
            lower(username);

            validateName(username);

            Identity id;
            id.fingerprint = toUUID(username);
            id.username = username;
            id.key = key;
            id.expansion   = asset(0'0000, S_EXP);
            id.created = now();
            id.activated = 0;

            return id;
        }

        static void validateName(string& username){
            string error = "Identity names must be between 3 and 20 characters, and contain only Letters, Numbers, and Dash (but not as the first or last characters).";
            check(username.length() >= 3 && username.length() <= 56, error.c_str());
            int i = 0;
            for ( char c : username ) {
                check(isalnum(c) || (c == '-' && (i != 0 && i != username.length()-1)), error.c_str());
                i++;
            }
        }
    };

    struct [[eosio::table, eosio::contract("ridlridlridl")]] Topup {
        uuid                    fingerprint;
        asset                   tokens;
        uint64_t                claimable;
        uuid primary_key() const { return fingerprint; }
    };

    typedef eosio::multi_index<"topups"_n, Topup> Topups;


    typedef eosio::multi_index<"ids"_n, Identity,
        indexed_by<"name"_n, const_mem_fun<Identity, uint64_t, &Identity::by_name>>
    > Identities;



}