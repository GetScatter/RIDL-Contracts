#include "ridl_token.hpp"

namespace ridl {

    void token::create(){
        require_auth( _self );

        name issuer = _self;
        asset maximum_supply = asset(ridl::MAX_SUPPLY, ridl::SYMBOL);

        stats statstable( _self, maximum_supply.symbol.code().raw() );
        auto existing = statstable.find( maximum_supply.symbol.code().raw() );
        check( existing == statstable.end(), "token with symbol already exists" );

        statstable.emplace( _self, [&]( auto& s ) {
            s.supply.symbol = maximum_supply.symbol;
            s.max_supply    = maximum_supply;
            s.issuer        = issuer;
        });

        SEND_INLINE_ACTION( *this, issue, {_self,"active"_n}, {"ridlridlridl"_n, asset( 500'000'000'0000, ridl::SYMBOL ), "RIDL Token Reserve"} );
    }


    void token::issue( name to, asset quantity, string memo ){
        check( memo.size() <= 256, "memo has more than 256 bytes" );

        stats statstable( _self, quantity.symbol.code().raw() );
        auto existing = statstable.find( quantity.symbol.code().raw() );
        check( existing != statstable.end(), "token with symbol does not exist, create token before issue" );
        const auto& st = *existing;

        require_auth( st.issuer );
        check( quantity.symbol.is_valid(), "invalid symbol name" );
        check( quantity.is_valid(), "invalid quantity" );
        check( quantity.amount > 0, "must issue positive quantity" );
        check( quantity.symbol == st.supply.symbol, "symbol precision mismatch" );
        check( quantity.amount <= st.max_supply.amount - st.supply.amount, "quantity exceeds available supply");

        statstable.modify( st, same_payer, [&]( auto& s ) {
            s.supply += quantity;
        });

        add_balance( st.issuer, quantity, st.issuer );

        if( to != st.issuer ) {
            SEND_INLINE_ACTION( *this, transfer, { {st.issuer, "active"_n} }, { st.issuer, to, quantity, memo } );
        }
    }

    void token::transfer(name from, name to, asset quantity, string memo){
        check( from != to, "cannot transfer to self" );
        require_auth( from );
        check( is_account( to ), "to account does not exist");
        stats statstable( _self, quantity.symbol.code().raw() );
        const auto& st = statstable.get( quantity.symbol.code().raw() );

        require_recipient( from );
        require_recipient( to );

        check( quantity.symbol.is_valid(), "invalid symbol name" );
        check( quantity.is_valid(), "invalid quantity" );
        check( quantity.amount > 0, "must transfer positive quantity" );
        check( quantity.symbol == st.supply.symbol, "symbol precision mismatch" );
        check( memo.size() <= 256, "memo has more than 256 bytes" );

        auto payer = has_auth( to ) ? to : from;

        sub_balance( from, quantity );
        add_balance( to, quantity, payer );
    }

    void token::movechainacc(name from, name to, asset quantity, string chain){
        movechain(from, quantity);
    }


    void token::movechainid(name from, string username, asset quantity){
        movechain(from, quantity);
    }

    void token::movedtokens(name to, asset quantity, string old_chain, string txid){
        SEND_INLINE_ACTION( *this, issue, { {_self, "active"_n} }, { to, quantity, txid } );
    }

    void token::movechain(name from, asset quantity){
        require_auth( from );
        stats statstable( _self, quantity.symbol.raw() );
        const auto& st = statstable.get( quantity.symbol.raw() );

        require_recipient( from );

        check( quantity.symbol.is_valid(), "invalid symbol name" );
        check( quantity.is_valid(), "invalid quantity" );
        check( quantity.amount > 0, "must transfer positive quantity" );
        check( quantity.symbol == st.supply.symbol, "symbol precision mismatch" );

        statstable.modify( st, same_payer, [&]( auto& s ) {
            s.supply -= quantity;
            s.max_supply -= quantity;
        });

        sub_balance( from, quantity );
    }

    void token::sub_balance( name owner, asset value ) {
        accounts from_acnts( _self, owner.value );

        const auto& from = from_acnts.get( value.symbol.code().raw(), "no balance object found" );
        check( from.balance.amount >= value.amount, "overdrawn balance" );

        from_acnts.modify( from, owner, [&]( auto& a ) {
            a.balance -= value;
        });
    }

    void token::add_balance( name owner, asset value, name ram_payer )
    {
        accounts to_acnts( _self, owner.value );
        auto to = to_acnts.find( value.symbol.code().raw() );
        if( to == to_acnts.end() ) {
            to_acnts.emplace( ram_payer, [&]( auto& a ){
                a.balance = value;
            });
        } else {
            to_acnts.modify( to, same_payer, [&]( auto& a ) {
                a.balance += value;
            });
        }
    }


}

EOSIO_DISPATCH( ridl::token, (create)(issue)(transfer)(movechainacc)(movechainid)(movedtokens) )