#include <eosiolib/eosio.hpp>
#include <eosiolib/singleton.hpp>


#include "lib/common.h"
#include "models/identity.h"
#include "models/balances.h"


#include "actions/balance_actions.hpp"
#include "actions/identity_actions.hpp"
#include "actions/reputation_actions.hpp"


using namespace eosio;
using namespace identity;
using namespace common;
using namespace balances;

using std::string;

CONTRACT ridlridlridl : contract {
public:
    using contract::contract;
    ridlridlridl(name receiver, name code,  datastream<const char*> ds):contract(receiver, code, ds) {}

    /***
     * // TODO: TESTING ONLY
     * ADMIN ONLY - Cleans all the tables
     */
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
    }






    /**********************************************/
    /***                                        ***/
    /***               Identities               ***/
    /***                                        ***/
    /**********************************************/

    /***
     * Registers an identity for a given account and name
     * @param account
     * @param username
     * @param key
     * @return
     */
    ACTION identify(const name& account, string& username, const public_key& key){
        IdentityActions(_self).identify(account, username, key);
    }

    /***
     * Changes the identity's public key
     * @param username
     * @param key
     * @return
     */
    ACTION changekey(string& username, const public_key& key){
        IdentityActions(_self).changekey(username, key);
    }

    /***
     * Changes the identity's linked account
     * @param username
     * @param key
     * @return
     */
    ACTION changeacc(string& username, name& account){
        IdentityActions(_self).changeacc(username, account);
    }

    /***
     * ADMIN ONLY - Adds a reserved identity to the table
     * @param username
     * @param key
     * @return
     */
    ACTION seed(string& username, const public_key& key){
        IdentityActions(_self).seed(username, key);
    }

    /***
     * Claims a reserved identity
     * @param account
     * @param username
     * @param key
     * @param sig
     * @return
     */
    ACTION claim(const name& account, string& username, const public_key& key, const signature& sig){
        IdentityActions(_self).claim(account, username, key, sig);
    }

    /***
     * Loads tokens into an identity
     * @param username
     * @param tokens
     * @return
     */
    ACTION topup(string& username, const asset& tokens){
        IdentityActions(_self).topup(username, tokens);
    }

    /***
     * Fallback for claiming topup
     * @param username
     * @return
     */
    ACTION applytopup(string& username){
        IdentityActions(_self).claimtopup(username);
    }





    /**********************************************/
    /***                                        ***/
    /***               Reputation               ***/
    /***                                        ***/
    /**********************************************/

    /***
     * Reputes an entity from a registered identity
     * @param username
     * @param entity
     * @param fragments
     * @param details (optional)
     * @return
     */
    ACTION repute(string& username, string& entity, std::vector<ReputationFragment>& fragments, const string& details){
        ReputationActions(_self).repute(username, entity, fragments);
    }

    /***
     * Votes a RepType into the database.
     * @param username - Voter's identity name
     * @param type - Type name
     * @param base (optional) - Entity fingerprint
     * @param upTag (optional)
     * @param downTag (optional)
     * @return
     */
//    ACTION votetype(string& username, string& type, string& base, string& upTag, string& downTag){
//        ReputationActions(_self).votetype(username, type, base, upTag, downTag);
//    }


    /***
     * ADMIN ONLY - Forces rep types into the table
     * @param type - Type name
     * @param base (optional) - Entity fingerprint
     * @param upTag (optional)
     * @param downTag (optional)
     * @return
     */
    ACTION forcetype(string& type, string& base, string& upTag, string& downTag){
        ReputationActions(_self).forcetype(type,base,upTag,downTag);
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
        EOSIO_DISPATCH_HELPER( ridlridlridl, (clean)(identify)(changekey)(changeacc)(seed)(claim)(topup)(applytopup)(repute)(forcetype) )
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