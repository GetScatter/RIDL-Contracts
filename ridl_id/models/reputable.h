#pragma once

#include "../lib/types.h"
#include "../lib/common.h"
#include "./identity.h"
#include "./reputation.h"
using namespace types;
using namespace common;
using namespace reputation;
using namespace identity;

using std::vector;
using std::string;
using std::optional;

namespace reputable {

    /*********************************/
    /*         MINER FRAGS           */
    /*********************************/
    struct [[eosio::table, eosio::contract("ridlridlridl")]] MinerFrag {
        uuid                    identity;
        uint64_t                miner_til;
        uuid                    fingerprint;
        uint8_t                 positive;

        uuid primary_key() const { return fingerprint; }
        uuid by_identity() const { return identity; }

        bool expired() const {
            return miner_til <= now();
        }

        static MinerFrag create(Identities::const_iterator& identity, ReputationFragment& frag){
            MinerFrag m;
            m.identity = identity->id;
            m.miner_til = now() + (SECONDS_PER_DAY * 30);
            m.fingerprint = frag.fingerprint;
            m.positive = frag.isPositive();
            return m;
        }
    };

    typedef eosio::multi_index<"minerfrags"_n, MinerFrag,
            indexed_by<"identity"_n, const_mem_fun<MinerFrag, uuid, &MinerFrag::by_identity>>
        >  MinerFrags;



    /*********************************/
    /*          REPUTABLES           */
    /*********************************/
    void validateEntity(string& type, string& entity){
        vector<string> entityTypes = vector<string>{"app", "acc", "act", "id", "etc"};

        eosio_assert(std::find(entityTypes.begin(), entityTypes.end(), type) != entityTypes.end(), "Invalid Entity Type");
        eosio_assert(entity.size() > 0, "Invalid entity");
    }

    void validateNetwork(string& network, string& type){
        eosio_assert(network.size() == 0 || type == "acc", "Only accounts/addresses/contracts can have a network ID");
        eosio_assert(network.size() == 0 || network.find("::", 0) != std::string::npos, "Invalid Network ID. Networks must be in the following format: [blockchain::chain_id]");
    }

    struct [[eosio::table, eosio::contract("ridlridlridl")]] Reputable {
        uint64_t                        id;
        string                          type;
        string                          entity;
        uuid                            base;
        uuid                            fingerprint;

        name                            last_reputer;
        uint64_t                        last_repute_time;
        name                            owner;
        string                          network;

        uint64_t                        block;

        uuid primary_key() const { return id; }
        uuid by_name() const { return toUUID(entity); }
        uint64_t by_fingerprint() const { return fingerprint; }
        uint64_t by_base() const { return base; }
        uint64_t by_owner() const { return owner.value; }

        void merge( const Reputable& r ){
            id = r.id;
            last_reputer = r.last_reputer;
            last_repute_time = r.last_repute_time;
            owner = r.owner;
            network = r.network;
        }

        static Reputable create(string& entity, string type, string network, uuid base){
            lower(entity);

            Reputable r;
            r.type = type;
            r.entity = entity;
            r.network = network;
            r.base = base;
            r.fingerprint = toUUID(type+entity+network+std::to_string(base));

            validateEntity(r.type, r.entity);
            validateNetwork(r.network, r.type);
            if(r.type == "id") eosio_assert(base == 0, "Identity reputes can not be tiered.");

            return r;
        }
    };




    typedef eosio::multi_index<"reputables"_n, Reputable,
        indexed_by<"name"_n, const_mem_fun<Reputable, uint64_t, &Reputable::by_name>>,
        indexed_by<"fingerprint"_n, const_mem_fun<Reputable, uint64_t, &Reputable::by_fingerprint>>,
        indexed_by<"base"_n, const_mem_fun<Reputable, uint64_t, &Reputable::by_base>>,
        indexed_by<"owner"_n, const_mem_fun<Reputable, uint64_t, &Reputable::by_owner>>
//        indexed_by<"miner"_n, const_mem_fun<Reputable, uint64_t, &Reputable::by_miner>>
        >  Reputables;



};