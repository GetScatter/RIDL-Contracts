#pragma once


#include <eosiolib/eosio.hpp>
#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/split.hpp>

#include "types.h"
using namespace types;

using namespace eosio;
using std::string;
using std::vector;

namespace common {

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
    void cleanTable(account_name self){
        T db(self, self);
        while(db.begin() != db.end()){
            auto itr = --db.end();
            db.erase(itr);
        }
    }

    vector<string> splitString(const string& str, const string& delimiter){
        std::vector<std::string> words;
        boost::split(words, str, boost::is_any_of(delimiter), boost::token_compress_on);
        return words;
    }

    vector<string> memoToApiParams(const string& memo){
        return splitString(memo, " ");
    }

    asset ridlToRep( asset& a ){
        return asset(a.amount, string_to_symbol(4, "REP"));
    }
}