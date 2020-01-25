#include <eosiolib/eosio.hpp>
#include <eosiolib/singleton.hpp>

using namespace eosio;

using std::string;

/***
 * This contract will provide decentralized user-aggregated
 * IBC for cross-chain RIDL reputations from the RIDL mainnet to
 * all other chains.
 *
 * IBC providers (anyone) will be able to gain REP by providing IBC services
 * to chains when running our RIDL IBC software/miner.
 */

CONTRACT ridlconnects : contract {
public:
    using contract::contract;
    ridlconnects(name receiver, name code,  datastream<const char*> ds):contract(receiver, code, ds) {}

    ACTION clean(name scope){
        require_auth(_self);

        print("Cleaning");
    }

    ACTION linkprovider(name& account, asset& rep, name& custodian){
        require_auth(custodian);
        eosio_assert(custodians.find(custodian) != custodians.end(), "This custodian is not registered with this contract.");
    }

    ACTION clonedata(name& account, string& type, vector<string>& stringData, vector<uint64_t>& intData64){
        require_auth(account);
    }







    /**********************************************/
    /***                                        ***/
    /***               Transfers                ***/
    /***                                        ***/
    /**********************************************/

    void transfer(const name& from, const name& to, const asset& quantity, const string& memo){
        if(to != _self) return;
        BalanceActions(_self).addBalance(from, quantity);
    }

};

extern "C" {
void apply(uint64_t receiver, uint64_t code, uint64_t action) {
    auto self = receiver;

    if( code == self ) switch(action) {
        EOSIO_DISPATCH_HELPER( ridlconnects, (clean)(identify)(changekey)(changeacc)(seed)(claim)(loadtokens)(tokensloaded)(repute)(forcetype) )
    }

    else {
        if(code == name("eosio.token").value && action == name("transfer").value){
            execute_action(name(receiver), name(code), &ridlconnects::transfer);
        }

        if(code == name("ridlridltkns").value && action == name("transfer").value){
            execute_action(name(receiver), name(code), &ridlconnects::transfer);
        }
    }
}
};