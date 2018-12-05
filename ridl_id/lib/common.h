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

    inline static uuid toUUID(string& username){
        return std::hash<string>{}(username);
    }

    void lower( string& anycase ){
        std::transform(anycase.begin(), anycase.end(), anycase.begin(), ::tolower);
    }

    void assertValidName(string& username){
        string error = "Identity names must be between 3 and 20 characters, and contain only Letters, Numbers, - _ and no spaces.";
        eosio_assert(username.length() >= 3 && username.length() <= 20, error.c_str());
        for ( char c : username ) eosio_assert(isalnum(c) || c == '-' || c == '_', error.c_str());
    }

    template <typename T>
    void cleanTable(name self){
        T db(self, self);
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
        return asset(a.amount, symbol("REP", 4));
    }

    struct transfer {
        name from;
        name to;
        asset quantity;
        string memo;
    };
};