#pragma once

#include "../lib/types.h"
#include "../lib/common.h"
using namespace types;
using namespace common;

using std::vector;
using std::string;

namespace bond {

    struct [[eosio::table, eosio::contract("ridlridlridl")]] Bond {
        uuid                id;
        uuid                identity;
        uuid                fingerprint;
        string              title;
        string              details;
        uint64_t            start_time;
        uint64_t            expires;
        asset               limit;
        asset               votes;
        uuid                fixed_party;
        uint8_t             closed;

        uuid primary_key() const { return id; }
        uint64_t by_identity() const {return identity; }
        uint64_t by_fingerprint() const { return fingerprint; }

        static Bond create(uuid identity, string& title, string& details, uint64_t duration, uint64_t starts_in_seconds, asset& limit){

            Bond bond;
            bond.identity = identity;
            bond.title = title;
            bond.details = details;
            bond.start_time = starts_in_seconds == 0 ? now() : now() + starts_in_seconds;
            bond.expires = now()+duration;
            bond.limit = limit;
            bond.votes = asset(0'0000, S_REP);
            bond.fixed_party = 0;
            bond.closed = 0;

            return bond;
        }

        bool isBusted() const {
            return votes.amount >= limit.amount;
        }

        bool hasFixedParty() const {
            return fixed_party > 0;
        }
    };

    typedef eosio::multi_index<"bonds"_n, Bond,
        indexed_by<"identity"_n, const_mem_fun<Bond, uint64_t, &Bond::by_identity>>,
        indexed_by<"fingerprint"_n, const_mem_fun<Bond, uint64_t, &Bond::by_fingerprint>>
        > Bonds;



}