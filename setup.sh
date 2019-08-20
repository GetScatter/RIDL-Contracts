#!/bin/bash

# add to ~/.bash_aliases
#alias "cleos=cleos -u http://ridlnet.get-scatter.com"
shopt -s expand_aliases
source ~/.bash_aliases

# Pass in any argument to only rebuild RIDL contract.
# Otherwise it will set up everything necessary on a clean EOSIO chain.

USER_KEY=EOS8YQzaYLxT17fWAPueQBxRjHehTQYZEvgPAWPPH4mAuwTJi3mPN

if [ "$#" -eq 0 ]; then
    # Basic Setup
    helpers/setup_nodeos.sh 1
    helpers/system_account.sh ridlridlcoin
    helpers/system_account.sh ridlridlridl
    helpers/system_account.sh ridlreserves
    helpers/system_account.sh test1account $USER_KEY
    helpers/system_account.sh test2account $USER_KEY


    # Token Setup
    ridl_token/scripts/set.sh
    cleos push action ridlridlcoin create '[]' -p ridlridlcoin
    cleos transfer eosio test1account "100000.0000 EOS" "" -p eosio
    cleos transfer eosio test2account "100000.0000 EOS" "" -p eosio

    # RIDL Contract Setup
    ridl_id/scripts/set.sh
    ridl_id/scripts/perms.sh
    ridl_id/scripts/set_types.sh

    # TEST IDENTITIES
    cleos transfer test1account ridlridlridl "10.0000 EOS" "" -p test1account
    cleos transfer test2account ridlridlridl "10.0000 EOS" "" -p test2account
    cleos push action ridlridlridl identify '["test1account", "HelloWorld", "'$USER_KEY'"]' -p test1account
    cleos push action ridlridlridl identify '["test2account", "HelloWorld2", "'$USER_KEY'"]' -p test2account
    cleos push action ridlridlridl identify '["test2account", "HelloWorld3", "'$USER_KEY'"]' -p test2account

fi

if [ "$#" -eq 1 ]; then
    ridl_id/scripts/set.sh
fi



