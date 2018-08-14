#include <eosiolib/eosio.hpp>
#include <eosiolib/singleton.hpp>
#include <eosiolib/currency.hpp>
#include <eosiolib/crypto.h>
#include "models/identity.h"
#include "lib/common.h"
#include "models/config.h"
#include "models/paid.h"

#include "actions/identity_actions.hpp"
#include "actions/reputation_actions.hpp"


using namespace eosio;
using namespace identity;
using namespace config;
using namespace common;
using namespace paid;

using std::string;

class ridl_id : contract {
public:
    using contract::contract;
    ridl_id( name self ) : contract(self){}


    // @abi action
    void clean(){
        require_auth(_self);

        print("Cleaning");

        ReputationTypes reputationTypes(_self, _self);
        while(reputationTypes.begin() != reputationTypes.end()){
            auto iter = --reputationTypes.end();
            string type = iter->type;
            FragTotals(_self, toUUID(type)).remove();
            reputationTypes.erase(iter);
        }

        Reputables reputables(_self,_self);
        while(reputables.begin() != reputables.end()){
            auto iter = --reputables.end();
            Reputations(_self, iter->fingerprint).remove();
            RepTotal(_self, iter->fingerprint).remove();
            reputables.erase(iter);
        }

        cleanTable<Identities>(_self);
        cleanTable<ReputationTypeVotes>(_self);
    }

    /**********************************************/
    /***                                        ***/
    /***                Configs                 ***/
    /***                                        ***/
    /**********************************************/

    // @abi action
    void setconfig( Config& config ){

        // Allow both self and the appointed custodian to update contract configs.
        account_name custodian = Configs(_self, _self).get_or_default(Config(_self)).custodian;
        eosio_assert(has_auth(_self) || has_auth(custodian), "Unauthorized");

        // Sets the configs object over the old one ( or initializes )
        Configs(_self, _self).set(config, _self);
    }






    /**********************************************/
    /***                                        ***/
    /***               Identities               ***/
    /***                                        ***/
    /**********************************************/


    // @abi action
    void identify(string& username, public_key& key, account_name& account){
        IdentityActions(_self).identify(username, key, account);
    }

    // @abi action
    void release(uuid& fingerprint, account_name& account, signature& sig){
        IdentityActions(_self).release(fingerprint, account, sig);
    }

    // @abi action
    void rekey(uuid& fingerprint, account_name& account, public_key& new_key){
        IdentityActions(_self).rekey(fingerprint, account, new_key);
    }

    // @abi action
    void setaccount(uuid& fingerprint, account_name& new_account, signature& sig){
        IdentityActions(_self).setaccount(fingerprint, new_account, sig);
    }

    // @abi action
    void seed(string& username, public_key& key){
        IdentityActions(_self).seed(username, key);
    }

    // @abi action
    void claim(string& username, public_key& key, signature& sig, account_name& account){
        IdentityActions(_self).claim(username, key, sig, account);
    }





    /**********************************************/
    /***                                        ***/
    /***               Reputation               ***/
    /***                                        ***/
    /**********************************************/

    // @abi action
    void repute(string& username, string& entity, std::vector<ReputationFragment>& fragments){
        ReputationActions(_self).repute(username, entity, fragments);
    }

    // @abi action
    void unrepute(string& username, string& entity){
        ReputationActions(_self).unrepute(username, entity);
    }

    // @abi action
    void votetype(string& username, string& type){
        ReputationActions(_self).votetype(username, type);
    }

    // @abi action
    void forcetype(string& type){
        ReputationActions(_self).forcetype(type);
    }




    /**********************************************/
    /***                                        ***/
    /***             Token Transfers            ***/
    /***                                        ***/
    /**********************************************/

    /***
     * Pays for an identity
     * @param t
     * @param code
     */
    void receivedTokens( const currency::transfer& t, account_name code ) {
        if( code == _self ){
            print("Contract sent money to itself?");
            return;
        }

        if( t.to == _self ) {
            if(code == N(eosio.token)) IdentityActions(_self).paidForIdentity(t);
            else if(code == N(ridlridlcoin)) IdentityActions(_self).loadedtokens(t);
            else eosio_assert(false, "This contract only accepts EOS or RIDL tokens");
        }

    }





    void apply( account_name contract, account_name action ) {
        if( action == N(transfer) ) {
            receivedTokens( unpack_action_data<eosio::currency::transfer>(), contract );
            return;
        }

        if( contract != _self ) return;
        auto& thiscontract = *this;
        switch( action ) {
            EOSIO_API( ridl_id, (clean)(setconfig)(identify)(release)(rekey)(setaccount)(seed)(claim)(repute)(unrepute)(votetype)(forcetype) )
        };
    }



};

// Can't use EOSIO_ABU macro since it doesn't support transfer catches.
extern "C" {
    [[noreturn]] void apply( uint64_t receiver, uint64_t code, uint64_t action ) {
        ridl_id c( receiver );
        c.apply( code, action );
        eosio_exit(0);
    }
}