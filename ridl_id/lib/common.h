#pragma once


#include <eosio/eosio.hpp>
#include <eosio/asset.hpp>
#include <eosio/transaction.hpp>
#include <eosio/crypto.hpp>

#include "types.h"
using namespace types;

using namespace eosio;
using std::string;
using std::vector;

namespace common {

    // CONSTANTS
    static const name           TOKEN_CONTRACT("ridlridltkns"_n);
    static const name           RESERVES_ACCOUNT("ridlreserves"_n);
    static const symbol         S_RIDL("RIDL", 4);
    static const symbol         S_EXP("EXP", 4);
    static const checksum256    RIDL_HASH = sha256("ridl", 4);
    static const uint64_t       SECONDS_PER_DAY(86400);
    static const uint64_t       TOPUP_DELAY(600); // TODO: configure to 1 day
    static const uint8_t        BLOCK_RANGE(1);

    // Checks that a given block is within range.
    void checkBlockNum(uint64_t& block_num){
        bool aboveRange = (block_num < tapos_block_num() + BLOCK_RANGE);
        bool belowRange = (block_num < tapos_block_num() - BLOCK_RANGE);
        check(aboveRange || belowRange, "Referenced block num is not within the target range. ("+std::to_string(tapos_block_num())+")");
    }

    public_key stringToKey(string const & str){
        public_key key;
        key.type = 0; // K1
        for(int i = 0; i < 33; ++i) key.data.at(i) = str.at(i);
        return key;
    }

    std::string intToString( uint64_t value ) {
        return std::to_string(value);
    }

    inline static uuid toUUID(string username){
        return std::hash<string>{}(username);
    }

    void lower( string& anycase ){
        std::transform(anycase.begin(), anycase.end(), anycase.begin(), ::tolower);
    }

    template <typename T>
    void cleanTable(name self, uint64_t scope = 0){
        uint64_t s = scope ? scope : self.value;
        T db(self, s);
        while(db.begin() != db.end()){
            auto itr = --db.end();
            db.erase(itr);
        }
    }

    void sendRIDL(name& from, name to, asset quantity, string memo){
        action( permission_level{ "ridlridlridl"_n, "active"_n },
                TOKEN_CONTRACT,
                name("transfer"),
                make_tuple(from, to, quantity, memo)
        ).send();
    }

    void cancelDeferred(uint64_t unique_id){
        // Temporary fix to cancel deferred transactions because cancelling them from the .send()
        // method throws an exception temporarily due to a patched but unapplied RAM exploit
        // https://github.com/EOSIO/eos/issues/6541
        cancel_deferred(unique_id);
    }

    void sendDeferred(name& _self, name action_name, uuid id, uint64_t delay_sec, uint64_t unique_id){
        cancelDeferred(unique_id);

        transaction t;
        t.actions.emplace_back(action( permission_level{ _self, "active"_n }, _self, action_name, std::make_tuple(id)));
        t.delay_sec = delay_sec;
        t.send(unique_id, _self, true);
    }

    uint32_t now(){
        return time_point_sec(current_time_point()).utc_seconds;
    }

};