#pragma once


#include <eosiolib/eosio.hpp>
#include <eosiolib/asset.hpp>

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
    static const uint64_t       SECONDS_PER_DAY(86400);
    static const uint64_t       TOPUP_DELAY(10);
    static const checksum256    RIDL_HASH = sha256("ridl", 4);

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
    void cleanTable(name self, name scope = name("")){
        name s = scope ? scope : self;
        T db(self, scope.value);
        while(db.begin() != db.end()){
            auto itr = --db.end();
            db.erase(itr);
        }
    }

    vector<string> splitString(const string& str, const string& delim){
        vector<string> tokens;
        size_t prev = 0, pos = 0;
        do
        {
            pos = str.find(delim, prev);
            if (pos == string::npos) pos = str.length();
            string token = str.substr(prev, pos-prev);
            if (!token.empty()) tokens.push_back(token);
            prev = pos + delim.length();
        }
        while (pos < str.length() && prev < str.length());
        return tokens;
    }

    asset ridlToRep( asset& a ){
        return asset(a.amount, S_REP);
    }

    void sendRIDL(name& from, name to, asset& quantity, string memo){
        action( permission_level{ "ridlridlridl"_n, "active"_n },
                name("ridlridlcoin"),
                name("transfer"),
                make_tuple(from, to, quantity, memo)
        ).send();
    }
};