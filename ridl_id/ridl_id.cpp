#include <eosio/eosio.hpp>
#include <eosio/singleton.hpp>
#include <eosio/binary_extension.hpp>


#include "lib/common.h"
#include "models/identity.h"
#include "models/config.h"
#include "models/reputation.h"


#include "actions/identity_actions.hpp"
#include "actions/reputation_actions.hpp"


using namespace eosio;
using namespace identity;
using namespace config;
using namespace common;
using namespace reputation;

using std::string;

CONTRACT ridlridlridl : contract {
public:
    using contract::contract;
    ridlridlridl(name receiver, name code,  datastream<const char*> ds):contract(receiver, code, ds) {}


    /**********************************************/
    /***                                        ***/
    /***               Managerial               ***/
    /***                                        ***/
    /**********************************************/

    /***
     * ADMIN ONLY - Cleans all tables
     */
    ACTION clean(){
        require_auth(_self);
        cleanTable<Identities>(_self, _self.value);
    }


    /***
     * ADMIN ONLY - Sets the identity creator auth account
     * @param account
     */
    ACTION setcreator(const name& account){
        require_auth(_self);
        Configs(_self, _self.value).set(configs({account}), _self);
    }

    /***
     * ADMIN ONLY - Adds a reserved identity to the table
     * @param username
     * @param key
     */
    ACTION seed(string& username, const public_key& key){
        IdentityActions(_self).seed(username, key);
    }

    /***
     * Revokes an identity
     * @param identity_id
     */
    ACTION revoke(uuid& identity_id){
        IdentityActions(_self).revoke(identity_id);
    }

    /***
     * Activates an identity
     * @param identity_id
     */
    ACTION activate(uuid& identity_id){
        IdentityActions(_self).activate(identity_id);
    }

    /***
     * Activates an identity
     * @param identity_id
     */
    ACTION buyactions(uuid& identity_id){
        IdentityActions(_self).buyactions(identity_id);
    }






    /**********************************************/
    /***                                        ***/
    /***               Identities               ***/
    /***                                        ***/
    /**********************************************/

    /***
     * Registers an identity for a given name
     * @param username
     * @param key
     */
    ACTION identify(string& username, const public_key& key){
        IdentityActions(_self).identify(username, key);
    }

    /***
     * Changes the identity's public key
     * @param username
     * @param key
     */
    ACTION changekey(uuid& identity_id, const string& key, uint64_t& block_num, const signature& sig){
        IdentityActions(_self).changekey(identity_id, key, block_num, sig);
    }

    /***
     * Fallback for claiming topup
     * @param username
     */
    ACTION tokensloaded(uuid& identity_id){
        IdentityActions(_self).tokensloaded(identity_id);
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
     * @param id (optional)
     * @param details (optional)
     */
    ACTION repute(uuid& identity_id, string& entity, vector<ReputationFragment>& fragments, asset& tokens, uint64_t& block_num, const signature& sig, name& sender, binary_extension<string> details){
        ReputationActions(_self).repute(identity_id, entity, fragments, tokens, block_num, sig, sender);
    }





    /**********************************************/
    /***                                        ***/
    /***               Transfers                ***/
    /***                                        ***/
    /**********************************************/

    void transfer(const name& from, const name& to, const asset& quantity, const string& memo){
        if(to != _self) return;
        IdentityActions(_self).loadtokens(from, quantity, memo);
    }

};

extern "C" {
void apply(uint64_t receiver, uint64_t code, uint64_t action) {
    auto self = receiver;

    if( code == self ) switch(action) {
        EOSIO_DISPATCH_HELPER( ridlridlridl, (clean)(setcreator)(seed)(revoke)(activate)(buyactions)(identify)(changekey)(tokensloaded)(repute) )
    }

    else {
        if(code == TOKEN_CONTRACT.value && action == name("transfer").value){
            execute_action(name(receiver), name(code), &ridlridlridl::transfer);
        }
    }
}
};