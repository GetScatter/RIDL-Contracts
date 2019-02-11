#pragma once

#include "../lib/types.h"
#include "../lib/common.h"
#include "./reputation.h"
using namespace types;
using namespace common;
using namespace reputation;

using std::vector;
using std::string;
using std::optional;

namespace reputable {

    void validateEntity(string& type, string& entity){
        vector<string> entityTypes = vector<string>{"app", "acc", "act", "id", "etc"};

        eosio_assert(std::find(entityTypes.begin(), entityTypes.end(), type) != entityTypes.end(), "Invalid Entity Type");
        eosio_assert(entity.size() > 0, "Invalid entity");
    }

    void validateNetwork(string& network_id, string& type){
        eosio_assert(network_id.size() == 0 || type == "acc", "Only accounts/addresses/contracts can have a network ID");
        eosio_assert(network_id.size() == 0 || network_id.find("::", 0) != std::string::npos, "Invalid Network ID. Networks must be in the following format: [blockchain::chain_id]");
    }

    struct [[eosio::table, eosio::contract("ridlridlridl")]] RepEntity {
        uuid                            id;
        uuid                            fingerprint;
        string                          type;
        string                          entity;
        uuid                            base;

        name                            miner;
        uint64_t                        miner_til;
        vector<ReputationFragment>      miner_frags;
        name                            last_reputer;
        uint64_t                        last_repute_time;
        name                            owner;
        asset                           total_rep;
        string                          network_id;

        uuid primary_key() const { return fingerprint; }
        uuid by_id() const { return id; }
        uuid by_name() const { return toUUID(entity); }
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


    static RepEntity create(string& entity, string& network_id, uuid& base){
        lower(entity);

        string fingermaker = entity + network_id + (base > 0 ? std::to_string(base) : "");

        RepEntity r;
        r.fingerprint = toUUID(fingermaker);
        r.type = entity.substr(0, entity.find("::", 0));
        r.entity = splitString(entity, "::")[1];
        r.miner_til = now() + (SECONDS_PER_DAY * 30);
        r.network_id = network_id;
        r.base = base;

        validateEntity(r.type, entity);
        validateNetwork(r.network_id, r.type);
        if(r.type == "id") eosio_assert(base == 0, "Identity reputes can not be tiered.");

        return r;
    }

    typedef eosio::multi_index<"reputables"_n, RepEntity,
        indexed_by<"name"_n, const_mem_fun<RepEntity, uint64_t, &RepEntity::by_name>>,
        indexed_by<"id"_n, const_mem_fun<RepEntity, uint64_t, &RepEntity::by_id>>,
        indexed_by<"owner"_n, const_mem_fun<RepEntity, uint64_t, &RepEntity::by_owner>>,
        indexed_by<"miner"_n, const_mem_fun<RepEntity, uint64_t, &RepEntity::by_miner>>
        >  Reputables;

}