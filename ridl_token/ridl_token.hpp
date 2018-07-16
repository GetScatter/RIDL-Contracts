#pragma once

#include <eosiolib/asset.hpp>
#include <eosiolib/eosio.hpp>

namespace ridl {

    using namespace eosio;
    using std::string;

    static uint64_t     SYMBOL = string_to_symbol(4, "RIDL");
    static int64_t      MAX_SUPPLY = 1'500'000'000'0000;

    class token : public contract {
    public:
        token( account_name self ):contract(self){}

        void create();

        void claim( account_name claimer );

        void issue( account_name to, asset quantity, string memo );

        void transfer( account_name from,
                       account_name to,
                       asset        quantity,
                       string       memo );


        inline asset get_supply( symbol_name sym )const;

        inline asset get_balance( account_name owner, symbol_name sym )const;

    private:
        struct account {
            asset    balance;

            uint64_t primary_key()const { return balance.symbol.name(); }
        };

        struct currency_stats {
            asset          supply;
            asset          max_supply;
            account_name   issuer;

            uint64_t primary_key()const { return supply.symbol.name(); }
        };

        typedef eosio::multi_index<N(accounts), account> accounts;
        typedef eosio::multi_index<N(stat), currency_stats> stats;

        void sub_balance( account_name owner, asset value );
        void add_balance( account_name owner, asset value, account_name ram_payer );

    public:

    };

    asset token::get_supply( symbol_name sym )const
    {
        stats statstable( _self, sym );
        const auto& st = statstable.get( sym );
        return st.supply;
    }

    asset token::get_balance( account_name owner, symbol_name sym )const
    {
        accounts accountstable( _self, owner );
        const auto& ac = accountstable.get( sym );
        return ac.balance;
    }

}