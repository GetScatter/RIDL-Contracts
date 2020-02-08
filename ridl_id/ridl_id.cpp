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
     * ADMIN ONLY - Sets the identity creator auth account
     * @param manager
     */
    ACTION init(const name& manager){
        check(is_account(manager), "The specified manager is not an existing account.");

        if(Configs(_self, 0).exists()){
            configs conf(Configs(_self, 0).get());
            require_auth(conf.manager);
            Configs(_self, 0).set(configs({manager, .starting_block = conf.starting_block}), _self);
        } else {
            require_auth(_self);
            Configs(_self, 0).set(configs({manager, .starting_block = static_cast<uint64_t>(tapos_block_num())}), _self);
        }
    }

    /***
     * ADMIN ONLY - Adds a reserved identity to the table
     * @param username
     * @param key
     */
    ACTION seed(string& username, const public_key& key, asset& tokens, asset& expansion){
        IdentityActions(_self).seed(username, key, tokens, expansion);
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
    ACTION buyactions(const uuid& identity_id, asset& quantity){
        IdentityActions(_self).buyactions(identity_id, quantity);
    }

    /***
     * Activates an identity
     * @param identity_id
     */
    ACTION movedtokens(string& username, asset& quantity, string& old_chain, string& txid){
        IdentityActions(_self).movedtokens(username, quantity);
    }

    /***
     * Moves an identity from one chain to another
     * @param identity_id
     */
    ACTION moveidentity(uuid& identity_id, string& new_chain_id){
        IdentityActions(_self).moveidentity(identity_id, new_chain_id);
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
        // Checking if the sender has a pending topup.
        IdentityActions(_self).tokensloaded(identity_id);
        ReputationActions(_self).repute(identity_id, entity, fragments, tokens, block_num, sig, sender);
    }





    /**********************************************/
    /***                                        ***/
    /***               Transfers                ***/
    /***                                        ***/
    /**********************************************/

    void transfer(const name& from, const name& to, const asset& quantity, const string& memo){
        if(to != _self) return;
        IdentityActions(_self).loadTokensFromTransfer(from, quantity, memo);
    }

};

extern "C" {
void apply(uint64_t receiver, uint64_t code, uint64_t action) {
    auto self = receiver;

    if( code == self ) switch(action) {
        EOSIO_DISPATCH_HELPER( ridlridlridl, (init)(seed)(revoke)(activate)(buyactions)(moveidentity)(identify)(changekey)(tokensloaded)(repute) )
    }

    else {
        if(code == TOKEN_CONTRACT.value && action == name("transfer").value){
            execute_action(name(receiver), name(code), &ridlridlridl::transfer);
        }
    }
}
};