#!/bin/bash

# Pass in any argument to only rebuild RIDL contract.
# Otherwise it will set up everything necessary on a clean EOSIO chain.

USER_KEY=EOS7w5aJCv5B7y3a6f4WCwPSvs6TpCAoRGnGpiLMsSWbmxaZdKigd

if [ "$#" -eq 0 ]; then
    # Basic Setup
    helpers/setup_nodeos.sh
    helpers/account.sh ridlridlcoin
    helpers/account.sh ridlridlridl
    helpers/account.sh scatterfunds
    helpers/account.sh ridlreserves
    helpers/account.sh test1account $USER_KEY


    # Token Setup
    ridl_token/scripts/set.sh
    cleos push action ridlridlcoin create '[]' -p ridlridlcoin
    cleos push action ridlridlcoin transfer '["scatterfunds", "test1account", "100000.0000 RIDL", ""]' -p scatterfunds
    cleos transfer eosio test1account "100000.0000 EOS" "" -p eosio
    cleos transfer eosio test1account "100000.0000 SYS" "" -p eosio

    # RIDL Contract Setup
    ridl_id/scripts/set.sh
    ridl_id/scripts/perms.sh
    ridl_id/scripts/seed.sh
    ridl_id/scripts/set_types.sh

fi

if [ "$#" -eq 1 ]; then
    ridl_id/scripts/set.sh
fi



