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
        name                    account;
        uint64_t                expires;
        asset                   tokens;
        asset                   total_rep;
        asset                   expansion;

        uuid primary_key() const { return id; }
        uint64_t by_name() const {return fingerprint; }
        uint64_t by_account() const {return account.value; }


        static Identity create(string& username, const public_key& key, const name& account){
            lower(username);

            validateName(username);

            Identity id;
            id.fingerprint = toUUID(username);
            id.username = username;
            id.key = key;
            id.account = account;
            id.expires = now() + (SECONDS_PER_DAY * 365);
            id.total_rep = asset(0'0000, S_REP);
            id.expansion = asset(0'0000, S_EXP);

            return id;
        }

        static void validateName(string& username){
            string error = "Identity names must be between 3 and 20 characters, and contain only Letters, Numbers, - _ and no spaces.";
            eosio_assert(username.length() >= 3 && username.length() <= 20, error.c_str());
            for ( char c : username ) eosio_assert(isalnum(c) || c == '-' || c == '_', error.c_str());
        }

        void authenticate() const {
            require_auth(account);
            eosio_assert(expires > now(), "Your Identity has expired.");
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
        indexed_by<"account"_n, const_mem_fun<Identity, uint64_t, &Identity::by_account>>,
        indexed_by<"name"_n, const_mem_fun<Identity, uint64_t, &Identity::by_name>>
        > Identities;



}