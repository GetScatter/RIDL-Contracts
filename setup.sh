#!/bin/bash

# add to ~/.bash_aliases
#alias "cleos=cleos -u http://ridlnet.get-scatter.com"
shopt -s expand_aliases
source ~/.bash_aliases

# Pass in any argument to only rebuild RIDL contract.
# Otherwise it will set up everything necessary on a clean EOSIO chain.

USER_KEY=EOS6MRyAjQq8ud7hVNYcfnVPJqcVpscN5So8BhtHuGYqET5GDW5CV

# Basic Setup
helpers/setup_nodeos.sh 1
sleep 3
helpers/system_account.sh ridlridltkns
helpers/system_account.sh ridlridlridl
helpers/system_account.sh ridlreserves
helpers/system_account.sh test1account $USER_KEY
helpers/system_account.sh test2account $USER_KEY


# Token Setup
ridl_token/scripts/set.sh
ridl_token/scripts/perms.sh
cleos push action ridlridltkns create '[]' -p ridlridltkns
cleos transfer eosio test1account "100000.0000 EOS" "" -p eosio
cleos transfer eosio test2account "100000.0000 EOS" "" -p eosio

# RIDL Contract Setup
ridl_id/scripts/set.sh
ridl_id/scripts/perms.sh





