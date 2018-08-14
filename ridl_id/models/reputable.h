#pragma once

#include "../lib/types.h"
#include "../lib/common.h"
#include "./reputation.h"
using namespace types;
using namespace common;

using std::vector;
using std::string;

namespace reputable {



    void validateEntity(string& type, string& entity){
        vector<string> entityTypes = vector<string>{"application", "contract", "identity"};

        eosio_assert(std::find(entityTypes.begin(), entityTypes.end(), type) != entityTypes.end(), "Invalid Entity Type");
        eosio_assert(entity.size() > 0, "Invalid entity");
    }

    // @abi table reputables
    struct RepEntity {
        uuid            fingerprint;
        string          type;
        string          entity;
        account_name    miner;
        uint64_t        miner_til;
        account_name    last_reputer;
        account_name    owner;

        void merge( const RepEntity& r ){
            miner_til = r.miner_til;
            miner = r.miner;
            last_reputer = r.last_reputer;
            owner = r.owner;
        }

        ~RepEntity(){}
        RepEntity(){}

        RepEntity(string _entity){
            lower(_entity);
            fingerprint = toUUID(_entity);
            type = _entity.substr(0, _entity.find("::", 0));
            entity = splitString(_entity, "::")[1];
            miner_til = now() + (seconds_per_day * 30);

            validateEntity(type, entity);
        }

        uuid primary_key() const { return fingerprint; }
        EOSLIB_SERIALIZE( RepEntity, (fingerprint)(type)(entity)(miner)(miner_til)(last_reputer)(owner) )
    };

    typedef eosio::multi_index<N(reputables), RepEntity>  Reputables;

}