#include <eosiolib/eosio.hpp>
#include <eosiolib/singleton.hpp>
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

CONTRACT ridlridlridl : contract {
public:
    using contract::contract;
    using eosio::unpack_action_data;

    ACTION clean(){
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

    ACTION setconfig( Config& config ){

        // Allow both self and the appointed custodian to update contract configs.
        name custodian = Configs(_self, _self).get_or_default(Config(_self)).custodian;
        eosio_assert(has_auth(_self) || has_auth(custodian), "Unauthorized");

        // Sets the configs object over the old one ( or initializes )
        Configs(_self, _self).set(config, _self);
    }






    /**********************************************/
    /***                                        ***/
    /***               Identities               ***/
    /***                                        ***/
    /**********************************************/


    ACTION identify(string& username, public_key& key, name& account){
        IdentityActions(_self).identify(username, key, account);
    }

    ACTION release(uuid& fingerprint, name& account, signature& sig){
        IdentityActions(_self).release(fingerprint, account, sig);
    }

    ACTION rekey(uuid& fingerprint, name& account, public_key& new_key){
        IdentityActions(_self).rekey(fingerprint, account, new_key);
    }

    ACTION setaccount(uuid& fingerprint, name& new_account, signature& sig){
        IdentityActions(_self).setaccount(fingerprint, new_account, sig);
    }

    ACTION seed(string& username, public_key& key){
        IdentityActions(_self).seed(username, key);
    }

    ACTION claim(string& username, public_key& key, signature& sig, name& account){
        IdentityActions(_self).claim(username, key, sig, account);
    }





    /**********************************************/
    /***                                        ***/
    /***               Reputation               ***/
    /***                                        ***/
    /**********************************************/

    // @abi action
    ACTION repute(string& username, string& entity, std::vector<ReputationFragment>& fragments){
        ReputationActions(_self).repute(username, entity, fragments);
    }

    ACTION unrepute(string& username, string& entity){
        ReputationActions(_self).unrepute(username, entity);
    }

    ACTION votetype(string& username, string& type){
        ReputationActions(_self).votetype(username, type);
    }

    ACTION forcetype(string& type){
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
    void receivedTokens( const currency::transfer& t, name code ) {
        if( code == _self ){
            print("Contract sent money to itself?");
            return;
        }

        if( t.to == _self ) {
            if(code == "eosio.token"_n) IdentityActions(_self).paidForIdentity(t);
            else if(code == "ridlridlcoin"_n) IdentityActions(_self).loadedtokens(t);
            else eosio_assert(false, "This contract only accepts EOS or RIDL tokens");
        }

    }





    void apply( name contract, name action ) {
        if( action == "transfer"_n ) {
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