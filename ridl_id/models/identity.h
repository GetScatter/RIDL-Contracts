#pragma once

#include "../lib/types.h"
#include "../lib/common.h"
using namespace types;
using namespace common;

using std::vector;
using std::string;

namespace identity {

    // States:
    // 0 Reputing
    // 1 Transferring

    // @abi table ids
    struct Identity {
        uuid                    fingerprint;
        string                  username;
        public_key              key;
        name                    account;
        uint64_t                expires;
        checksum256             hash;
        uint8_t                 state;
        asset                   tokens;


        uuid primary_key() const { return fingerprint; }

        ~Identity(){}
        Identity(){}

        Identity(string& _username, public_key& _key, name& _account){
            lower(_username);
            fingerprint = toUUID(_username);
            username = _username;
            key = _key;
            account = _account;
            expires = now() + (seconds_per_day * 365);

            hash = sha256((char *) &username, sizeof(username));
        }

        /***
         * Proves that a signature came from someone who owns this Identity's private key.
         * @param sig
         */
        void prove( const signature& sig ) const {
            assert_recover_key( hash, sig, key );
        }
    };








    // @abi table namerefs
    struct NameReference {
        uuid                    key;
        vector<string>          usernames;

        uint64_t primary_key() const { return key; }
    };


    typedef eosio::multi_index<"ids"_n,   Identity>              Identities;
    typedef eosio::multi_index<"namerefs"_n,   NameReference>    NameReferences;




}