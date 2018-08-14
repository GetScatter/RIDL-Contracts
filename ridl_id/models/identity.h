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
        account_name            account;
        uint64_t                expires;
        checksum256             hash;
        uint8_t                 state;
        eosio::asset            tokens;


        uuid primary_key() const { return fingerprint; }
        EOSLIB_SERIALIZE( Identity, (fingerprint)(username)(key)(account)(expires)(hash)(state)(tokens) )

        ~Identity(){}
        Identity(){}

        Identity(string& _username, public_key& _key, account_name& _account){
            lower(_username);
            fingerprint = toUUID(_username);
            username = _username;
            key = _key;
            account = _account;
            expires = now() + (seconds_per_day * 365);

            sha256((char *) &username, sizeof(username), &hash);
        }

        /***
         * Proves that a signature came from someone who owns this Identity's private key.
         * @param sig
         */
        void prove( const signature& sig ) const {
            assert_recover_key( &hash,
                                (const char*)&sig, sizeof(sig),
                                (const char*)key.data,
                                sizeof(key) );
        }
    };








    // @abi table namerefs
    struct NameReference {
        uuid                    key;
        vector<string>          usernames;

        uint64_t primary_key() const { return key; }
        EOSLIB_SERIALIZE( NameReference, (key)(usernames) )
    };


    typedef eosio::multi_index<N(ids),   Identity>              Identities;
    typedef eosio::multi_index<N(namerefs),   NameReference>    NameReferences;




}