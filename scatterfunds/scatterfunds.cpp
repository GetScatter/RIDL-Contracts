#include <eosiolib/eosio.hpp>
#include <eosiolib/currency.hpp>
#include <eosiolib/singleton.hpp>
#include <eosio.token.hpp>
#include <../ridl_token/ridl_token.hpp>

using namespace eosio;
using std::string;

class scatterfunds : contract {
private:

    // 2 years ( 730 days * 2 because of 12hr intervals )
    static const int64_t lastCycle = 1459;

    asset tokensPerCycle(){
        return asset(170000'0000, string_to_symbol(4, "RIDL"));
    }

    asset maxEosPerCycle(){
        return asset(2000'0000, string_to_symbol(4, "EOS"));
    }

    // @abi table claimables
    // @abi table cycles
    struct CycleData {
        int64_t   cycle;
        asset     tokens;

        int64_t primary_key() const { return cycle; }
        EOSLIB_SERIALIZE( CycleData, (cycle)(tokens) )
    };

    // @abi table settings
    struct settings {
        int64_t   setting;

        int64_t primary_key() const { return setting; }
        EOSLIB_SERIALIZE( settings, (setting) )
    };



    typedef multi_index<N(claimables), CycleData>    Claimables;
    typedef singleton<  N(cycles),     CycleData>    Cycles;
    typedef singleton<  N(settings),   settings>     Settings;




    int64_t getCurrentCycle(){
        int64_t startTime = Settings(_self, N("started")).get().setting;
        return ((now() - startTime) / 3600) / 12;
//        return 2;
    }

    CycleData getCycleData(int64_t cycle){
        Cycles cycles(_self, cycle);
        return cycles.get_or_default(CycleData{cycle, asset(0'0000, string_to_symbol(4, "EOS"))});
    }

    void setCycleData(int64_t cycle, asset quantity){
        Cycles cycles(_self, cycle);
        CycleData cycleData = getCycleData(cycle);
        cycleData.tokens = quantity;
        cycles.set(cycleData, _self);
    }

public:
    using contract::contract;
    scatterfunds( name self ) : contract(self){}

    // @abi action
    void start(){
        require_auth(_self);
        eosio_assert(!Settings(_self, N("started")).exists(), "Already Started");
        Settings(_self, N("started")).set(settings{now()}, _self);
    }

    // @abi actions
    void claim( account_name owner ){
        Claimables claimables(_self, owner);
        eosio_assert(claimables.begin() != claimables.end(), "Account has nothing to claim");

        asset dispensable(0'0000, string_to_symbol(4, "RIDL"));

        uint64_t currentCycle = getCurrentCycle();
        asset perCycle = tokensPerCycle();


        auto claimable = claimables.begin();
        while(claimable != claimables.end()){
            if(claimable->cycle < currentCycle){
                asset total = getCycleData(claimable->cycle).tokens;
                float percentage = (float)claimable->tokens.amount / total.amount;
                dispensable += asset(perCycle.amount*percentage, string_to_symbol(4, "RIDL"));
                claimables.erase(claimable);
                claimable = claimables.begin();
            } else claimable++;
        }

        eosio_assert(dispensable.amount > 0, "Not ready to dispense yet.");
        INLINE_ACTION_SENDER(ridl::token, transfer)( N(ridlridlcoin), {{_self,N(active)}}, {_self, owner, dispensable, "RIDL Claimed"} );
    }





    /**********************************************/
    /***                                        ***/
    /***             Token Transfers            ***/
    /***                                        ***/
    /**********************************************/

    void buy( const currency::transfer& t ){

        eosio_assert(t.quantity.symbol == string_to_symbol(4, "EOS"), "Token must be EOS");
        eosio_assert(t.quantity.is_valid(), "Token asset is not valid");
        eosio_assert(t.quantity.amount >= 1'0000, "Not enough tokens");

        int64_t cycle(getCurrentCycle());
        eosio_assert(cycle <= lastCycle, "2 years have passed, this development fundraiser is over.");

        CycleData cycleData = getCycleData(cycle);
        asset total = cycleData.tokens;
        eosio_assert(total < maxEosPerCycle(), "Too much has been spent today");

        asset quantity = t.quantity;

        // Too much was spent, sending the overage back
        if(quantity + total > maxEosPerCycle()){
            asset refunded = quantity + total - maxEosPerCycle();
            quantity = quantity - refunded;
            INLINE_ACTION_SENDER(eosio::token, transfer)( N(eosio.token), {{_self,N(active)}}, {_self, t.from, refunded, "RIDL Over-Spend Refund"} );
        }

        setCycleData(cycle, total + quantity);

        Claimables claimables(_self, t.from);
        auto iter = claimables.find(cycle);
        if(iter == claimables.end()) claimables.emplace(_self, [&](auto& row){
                row.cycle = cycle;
                row.tokens = quantity;
            });
        else claimables.modify(iter, 0, [&](auto& row){
                row.tokens += quantity;
            });
    }

    void receivedTokens( const currency::transfer& t, account_name code ) {
        if( code == _self ){
            print("Contract sent money to itself?");
            return;
        }

        if( t.to == _self ) {
            if (code == N(eosio.token)) {
                if(t.from != N(eosio.stake)) {
                    buy(t);
                }
            }
            else eosio_assert(false, "This contract only accepts EOS and RIDL tokens");
        }
    }

    void apply( account_name contract, account_name action ) {
        if( action == N(transfer) ) {
            receivedTokens( unpack_action_data<eosio::currency::transfer>(), contract );
            return;
        }

        if( action == N(buy) ){
            eosio_assert(false, "Can't call buy directly");
        }

        if( contract != _self ) return;
        auto& thiscontract = *this;
        switch( action ) {
            EOSIO_API( scatterfunds, (start)(claim) )
        };
    }



};

extern "C" {
    [[noreturn]] void apply( uint64_t receiver, uint64_t code, uint64_t action ) {
        scatterfunds c( receiver );
        c.apply( code, action );
        eosio_exit(0);
    }
}