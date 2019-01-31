#pragma once


#include <eosiolib/eosio.hpp>
#include <eosiolib/asset.hpp>

#include "types.h"
using namespace types;

using namespace eosio;
using std::string;
using std::vector;

namespace common {

    static const symbol S_RIDL = symbol("RIDL", 4);
    static const symbol S_REP = symbol("REP", 4);
    static const symbol S_EOS = symbol("SYS", 4);
    static const uint64_t seconds_per_day(86400);

    // sha256 of "ridl"
    static const checksum256 ridlHash = sha256("ridl", 4);

    static void prove( const signature& sig, const public_key& key ) {
        assert_recover_key(ridlHash, sig, key);
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

    vector<string> memoToApiParams(const string& memo){
        return splitString(memo, " ");
    }

    asset ridlToRep( asset& a ){
        return asset(a.amount, S_REP);
    }

    struct transfer {
        name from;
        name to;
        asset quantity;
        string memo;
    };
};