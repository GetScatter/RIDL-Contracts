#include <eosiolib/eosio.hpp>
#include <eosiolib/singleton.hpp>


#include "lib/common.h"
#include "models/identity.h"
#include "models/config.h"
#include "models/balances.h"


#include "actions/balance_actions.hpp"
#include "actions/identity_actions.hpp"
#include "actions/reputation_actions.hpp"


using namespace eosio;
using namespace identity;
using namespace config;
using namespace common;
using namespace balances;

using std::string;

CONTRACT ridlridlridl : contract {
public:
    using contract::contract;
    ridlridlridl(name receiver, name code,  datastream<const char*> ds):contract(receiver, code, ds) {}

    ACTION clean(name scope){
        require_auth(_self);

        print("Cleaning");

        ReputationTypes reputationTypes(_self, _self.value);
        while(reputationTypes.begin() != reputationTypes.end()){
            auto iter = --reputationTypes.end();
            string type = iter->type;
            FragTotals(_self, toUUID(type)).remove();
            reputationTypes.erase(iter);
        }

        Reputables reputables(_self,_self.value);
        while(reputables.begin() != reputables.end()){
            auto iter = --reputables.end();
            Reputations(_self, iter->fingerprint).remove();
            reputables.erase(iter);
        }

        cleanTable<Identities>(_self, _self);
        cleanTable<Balances>(_self, scope);
//        cleanTable<ReputationTypeVotes>(_self);

        Configs(_self, _self.value).remove();
    }

    /**********************************************/
    /***                                        ***/
    /***                Configs                 ***/
    /***                                        ***/
    /**********************************************/

    ACTION setconfig( configs& config ){
        require_auth(_self);

        Configs(_self, _self.value).set(config, _self);
    }






    /**********************************************/
    /***                                        ***/
    /***               Identities               ***/
    /***                                        ***/
    /**********************************************/


    ACTION identify(const name& account, string& username, const public_key& key){
        IdentityActions(_self).identify(account, username, key);
    }

    ACTION release(string& username, const signature& sig){
        IdentityActions(_self).release(username, sig);
    }

    ACTION rekey(string& username, const public_key& key){
        IdentityActions(_self).rekey(username, key);
    }

    ACTION setaccacc(string& username, const name& new_account){
        IdentityActions(_self).setaccountacc(username, new_account);
    }

    ACTION setacckey(string& username, const name& new_account, const signature& sig){
        IdentityActions(_self).setaccountkey(username, new_account, sig);
    }

    ACTION seed(string& username, const public_key& key){
        IdentityActions(_self).seed(username, key);
    }

    ACTION claim(const name& account, string& username, const public_key& key, const signature& sig){
        IdentityActions(_self).claim(account, username, key, sig);
    }

    ACTION loadtokens(const name& account, string& username, const asset& tokens){
        IdentityActions(_self).loadtokens(account, username, tokens);
    }





    /**********************************************/
    /***                                        ***/
    /***               Reputation               ***/
    /***                                        ***/
    /**********************************************/

    ACTION repute(string& username, string& entity, std::vector<ReputationFragment>& fragments, const string& details){
        ReputationActions(_self).repute(username, entity, fragments);
    }

//    ACTION votetype(string& username, string& type){
//        ReputationActions(_self).votetype(username, type);
//    }

    ACTION forcetype(string& type, string& base){
        ReputationActions(_self).forcetype(type,base);
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
        EOSIO_DISPATCH_HELPER( ridlridlridl, (clean)(setconfig)(identify)(release)(rekey)(setaccacc)(setacckey)(seed)(claim)(loadtokens)(repute)(forcetype) )
    }

    else {
        if(code == name("eosio.token").value && action == name("transfer").value){
            execute_action(name(receiver), name(code), &ridlridlridl::transfer);
        }

        if(code == name("ridlridlcoin").value && action == name("transfer").value){
            execute_action(name(receiver), name(code), &ridlridlridl::transfer);
        }
    }


}
};