#!/bin/bash

shopt -s expand_aliases
source ~/.bash_aliases

eosio-cpp --abigen --contract=ridlridlridl ./ridl_id/ridl_id.cpp -o ~/contracts/ridl_id.wasm
sed -i 's/1.1/1.0/g' ~/contracts/ridl_id.abi
cleos set contract ridlridlridl ~/contracts/ ridl_id.wasm ridl_id.abi -p ridlridlridl@active