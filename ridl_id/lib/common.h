#pragma once


#include <eosiolib/eosio.hpp>
#include <eosiolib/asset.hpp>
#include <eosiolib/transaction.hpp>

#include "types.h"
using namespace types;

using namespace eosio;
using std::string;
using std::vector;

namespace common {

    // CONSTANTS
    static const symbol         S_RIDL("RIDL", 4);
    static const symbol         S_REP("REP", 4);
    static const symbol         S_EOS("EOS", 4);
    static const symbol         S_EXP("EXP", 4);
    static const checksum256    RIDL_HASH = sha256("ridl", 4);
    static const checksum256    NO_HASH = sha256("", 0);
    static const uint64_t       SECONDS_PER_DAY(86400);
    static const uint64_t       TOPUP_DELAY(600); // TODO: configure to 1 day
    static const uint64_t       MIN_REQUIRED_REP_FOR_BONDS(500);

    static void prove( const signature& sig, const public_key& key ) {
        assert_recover_key(RIDL_HASH, sig, key);
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

    vector<string> splitString(const string& str, const string& delim){
        vector<string> parts;
        if(str.size() == 0) return parts;
        size_t prev = 0, pos = 0;
        do
        {
            pos = str.find(delim, prev);
            if (pos == string::npos) pos = str.length();
            string token = str.substr(prev, pos-prev);
            if (!token.empty()) parts.push_back(token);
            prev = pos + delim.length();
        }
        while (pos < str.length() && prev < str.length());
        return parts;
    }

    asset ridlToRep( asset& a ){
        return asset(a.amount, S_REP);
    }

    void sendRIDL(name& from, name to, asset quantity, string memo){
        action( permission_level{ "ridlridlridl"_n, "active"_n },
                name("ridlridlcoin"),
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

};