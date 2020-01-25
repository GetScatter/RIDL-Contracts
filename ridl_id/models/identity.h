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
        uint64_t                expires;
        asset                   tokens;
        asset                   expansion;
        uint64_t                created;
        uint64_t                block;

        uuid primary_key() const { return id; }
        uint64_t by_name() const {return fingerprint; }


        static Identity create(string& username, const public_key& key){
            lower(username);

            validateName(username);

            Identity id;
            id.fingerprint = toUUID(username);
            id.username = username;
            id.key = key;
            id.expires = now() + (SECONDS_PER_DAY * 365);
            id.expansion   = asset(0'0000, S_EXP);
            id.created = now();

            return id;
        }

        static void validateName(string& username){
            string error = "Identity names must be between 3 and 20 characters, and contain only Letters, Numbers, - _ and no spaces.";
            check(username.length() >= 3 && username.length() <= 20, error.c_str());
            for ( char c : username ) check(isalnum(c) || c == '-' || c == '_', error.c_str());
        }

        void authenticate(uint64_t& block_num, const signature& sig) const {
            checkBlockNum(block_num);
            string cleartext = (std::to_string(this->id) + std::to_string(block_num)).c_str();
            checksum256 hash = sha256(cleartext.c_str(), cleartext.size());
            assert_recover_key(hash, sig, this->key);
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