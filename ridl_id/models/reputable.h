#pragma once

#include "../lib/types.h"
#include "../lib/common.h"
#include "./reputation.h"
using namespace types;
using namespace common;
using namespace reputation;

using std::vector;
using std::string;

namespace reputable {

    void validateEntity(string& type, string& entity){
        vector<string> entityTypes = vector<string>{"app", "acc", "id"};

        eosio_assert(std::find(entityTypes.begin(), entityTypes.end(), type) != entityTypes.end(), "Invalid Entity Type");
        eosio_assert(entity.size() > 0, "Invalid entity");
    }

    struct [[eosio::table, eosio::contract("ridlridlridl")]] RepEntity {
        uuid                            id;
        uuid                            fingerprint;
        string                          type;
        string                          entity;

        name                            miner;
        uint64_t                        miner_til;
        vector<ReputationFragment>      miner_frags;
        name                            last_reputer;
        name                            owner;
        asset                           total_rep;

        uuid primary_key() const { return fingerprint; }
        uint64_t by_id() const { return id; }
        uint64_t by_owner() const { return owner.value; }
        uint64_t by_miner() const { return miner.value; }

        void merge( const RepEntity& r ){
            miner_til = r.miner_til;
            miner = r.miner;
            miner_frags = r.miner_frags;
            last_reputer = r.last_reputer;
            owner = r.owner;
            total_rep += r.total_rep;
        }

        bool hasMinerFragment( ReputationFragment& frag ){
            string type = frag.type;

            // Frag exists
            auto found = std::find_if(
                    miner_frags.begin(),
                    miner_frags.end(),
                    [type](const ReputationFragment & f) -> bool
                    { return f.type == type; }
            );

            return found != miner_frags.end();
        }
    };


    static RepEntity create(string& entity){
        lower(entity);

        RepEntity r;
        r.fingerprint = toUUID(entity);
        r.type = entity.substr(0, entity.find("::", 0));
        r.entity = splitString(entity, "::")[1];
        r.miner_til = now() + (seconds_per_day * 30);

        validateEntity(r.type, entity);

        return r;
    }

    typedef eosio::multi_index<"reputables"_n, RepEntity,
        indexed_by<"id"_n, const_mem_fun<RepEntity, uint64_t, &RepEntity::by_id>>,
        indexed_by<"owner"_n, const_mem_fun<RepEntity, uint64_t, &RepEntity::by_owner>>,
        indexed_by<"miner"_n, const_mem_fun<RepEntity, uint64_t, &RepEntity::by_miner>>
        >  Reputables;

}