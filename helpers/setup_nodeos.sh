#!/bin/bash

# Sets up nodeos after blowing it away or from a fresh install.
# Uses the standard EOSIO public key to initialize.
# Defaults the the `eos` directory to `~/eosio.contracts` if no path is specified

# !!!FAQ!!! You might need to run this twice.
# It seems to time out on setting the eosio.system contract.

# nodeos --delete-all-blocks --genesis-json ./genesis.json

EOS_BASE_PATH="$HOME/eosio.contracts"

if [ "$#" -eq 1 ]; then
    EOS_BASE_PATH=$1
fi

PKEY=EOS6MRyAjQq8ud7hVNYcfnVPJqcVpscN5So8BhtHuGYqET5GDW5CV



cleos create account eosio eosio.ram $PKEY $PKEY -p eosio
cleos create account eosio eosio.ramfee $PKEY $PKEY -p eosio
cleos create account eosio eosio.stake $PKEY $PKEY -p eosio
cleos create account eosio eosio.token $PKEY $PKEY -p eosio

cleos set contract eosio.token $EOS_BASE_PATH/eosio.token --abi eosio.token.abi -p eosio.token
cleos push action eosio.token create '[ "eosio", "1000000000.0000 SYS"]' -p eosio.token
cleos push action eosio.token issue '[ "eosio", "1000000000.0000 SYS", "init" ]' -p eosio
cleos push action eosio.token create '[ "eosio", "1000000000.0000 EOS"]' -p eosio.token
cleos push action eosio.token issue '[ "eosio", "1000000000.0000 EOS", "init" ]' -p eosio

# Only using when testing resources. Otherwise no point in that overhead.
# cleos set contract eosio $EOS_BASE_PATH/eosio.system -p eosio
# cleos push action eosio setram '{"max_ram_size":"64599818083"}' -p eosio