#include <eosiolib/eosio.hpp>
#include <eosiolib/singleton.hpp>


#include "lib/common.h"
#include "models/identity.h"
#include "models/balances.h"
#include "models/bond.h"
#include "models/reputable.h"


#include "actions/balance_actions.hpp"
#include "actions/identity_actions.hpp"
#include "actions/reputation_actions.hpp"
#include "actions/bond_actions.hpp"


using namespace eosio;
using namespace identity;
using namespace common;
using namespace balances;
using namespace bond;
using namespace reputable;

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
            Reputations(_self, iter->id).remove();
            cleanTable<MinerFrags>(_self, iter->id);
            cleanTable<MinerFrags>(_self, iter->id);

            reputables.erase(iter);
        }

        cleanTable<Identities>(_self, _self.value);
        cleanTable<Bonds>(_self, _self.value);
        cleanTable<Balances>(_self, scope.value);
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
    ACTION loadtokens(string& username, const asset& tokens){
        IdentityActions(_self).loadtokens(username, tokens);
    }

    /***
     * Fallback for claiming topup
     * @param username
     * @return
     */
    ACTION tokensloaded(uuid& identity_id){
        IdentityActions(_self).tokensloaded(identity_id);
    }




    /**********************************************/
    /***                                        ***/
    /***                  Bonds                 ***/
    /***                                        ***/
    /**********************************************/

    /***
     * Creates a bond
     * @param username
     * @param title
     * @param details
     * @param duration
     * @param starts_in_seconds
     * @param limit
     * @param fixed_party
     * @return
     */
    ACTION createbond(string& username, string& title, string& details, uint64_t duration, uint64_t starts_in_seconds, asset& limit, uuid& fixed_party){
        BondActions(_self).create(username, title, details, duration, starts_in_seconds, limit, fixed_party);
    }

    /***
     * Votes on a bond
     * @param username
     * @param bond_id
     * @param rep
     * @return
     */
    ACTION disputebond(string& username, uuid& bond_id, asset rep){
        BondActions(_self).disputebond(username, bond_id, rep);
    }

    /***
     * Allows a fixed_party to cancel a bond for the bonder
     * @param username
     * @param bond_id
     * @return
     */
    ACTION cancelbond(string& username, uuid& bond_id){
        BondActions(_self).cancelbond(username, bond_id);
    }

    /***
     * Closes a limit reached or expired bond
     * !! CALLED FROM DEFERRED OR FALLBACK !!
     * @param bond_id
     * @return
     */
    ACTION closebond(uuid& bond_id){
        BondActions(_self).closebond(bond_id);
    }

    /***
     * Erases a closed bond
     * @param bond_id
     * @return
     */
    ACTION erasebond(uuid& bond_id){
        BondActions(_self).erasebond(bond_id);
    }



    /**********************************************/
    /***                                        ***/
    /***               Reputation               ***/
    /***                                        ***/
    /**********************************************/

    /***
     * Reputes an entity from a registered identity
     * @param username
     * @param id
     * @param entity
     * @param type
     * @param fragments
     * @param network
     * @param parent
     * @param details
     * @return
     */
    ACTION repute(string& username, uuid& id, string& entity, string& type, vector<ReputationFragment>& fragments, string& network, string& parent, string& details){
        ReputationActions(_self).repute(username, id, entity, type, fragments, network, parent);
    }

    /***
     * Votes a RepType into the database.
     * @param username - Voter's identity name
     * @param type - Type name
     * @param parent (optional) - Entity fingerprint
     * @param upTag (optional)
     * @param downTag (optional)
     * @return
     */
     // TODO: Need to implement decentralized voting on rep types
//    ACTION votetype(string& username, string& type, string& parent, string& upTag, string& downTag){
//        ReputationActions(_self).votetype(username, type, parent, upTag, downTag);
//    }


    /***
     * ADMIN ONLY - Forces rep types into the table
     * @param type - Type name
     * @param parent (optional) - Entity fingerprint
     * @param upTag (optional)
     * @param downTag (optional)
     * @return
     */
    ACTION forcetype(string& type, uuid& parent, string& upTag, string& downTag){
        ReputationActions(_self).forcetype(type,parent,upTag,downTag);
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
        EOSIO_DISPATCH_HELPER( ridlridlridl, (clean)(identify)(changekey)(changeacc)(seed)(claim)(loadtokens)(tokensloaded)(createbond)(disputebond)(cancelbond)(closebond)(erasebond)(repute)(forcetype) )
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