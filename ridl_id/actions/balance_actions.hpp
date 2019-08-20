#pragma once

#include <eosiolib/eosio.hpp>

#include "../models/balances.h"
using namespace balances;

class BalanceActions {
public:
    name _self;
    BalanceActions(name self):_self(self){}

    void hasBalance(const name& account, const asset& quantity){
        Balances balances(_self, account.value);
        auto iterator = balances.find(quantity.symbol.raw());
        eosio_assert(iterator != balances.end(), "Could not find balance");
        eosio_assert(iterator->balance.amount >= quantity.amount, "Not enough tokens.");
    }

    void addBalance(const name& account, const asset& quantity){
        Balances balances(_self, account.value);
        auto iterator = balances.find(quantity.symbol.raw());
        if(iterator == balances.end()) balances.emplace(_self, [&](auto& row){
            row.symbol = quantity.symbol.raw();
            row.balance = quantity;
        });
        else balances.modify(iterator, same_payer, [&](auto& row){
            row.balance += quantity;
        });
    }

    void subBalance(const name& account, const asset& quantity){
        Balances balances(_self, account.value);
        auto iterator = balances.find(quantity.symbol.raw());

        eosio_assert(iterator != balances.end(), "No balance object");
        eosio_assert(iterator->balance.amount >= quantity.amount, "overdrawn balance" );

        if(iterator->balance.amount - quantity.amount <= 0){
            balances.erase(iterator);
        }

        else balances.modify(iterator, same_payer, [&](auto& row){
            row.balance -= quantity;
        });
    }

};
