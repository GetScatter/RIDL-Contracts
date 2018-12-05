#!/bin/bash

#~/setup_nodeos.sh
../account.sh ridlridlcoin
../account.sh ridlridlridl
../account.sh scatterfunds
../account.sh ridlreserves
../account.sh test1account

./set.sh
#comptract.sh ridl_token ridlridlcoin

cleos push action ridlridlcoin create '[]' -p ridlridlcoin

//cleos transfer eosio test1account "10000.0000 EOS" "" -p eosio
