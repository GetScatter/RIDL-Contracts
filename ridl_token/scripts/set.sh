#!/bin/bash

shopt -s expand_aliases
source ~/.bash_aliases

# eosio-cpp --abigen --contract=token ridl_token.cpp -o ridl_token.wasm
# cleos set contract ridlridltkns . ridl_token.wasm ridl_token.abi -p ridlridltkns@active

eosio-cpp --abigen --contract=token ./ridl_token/ridl_token.cpp -o ~/contracts/ridl_token.wasm
sed -i 's/1.1/1.0/g' ~/contracts/ridl_token.abi
cleos set contract ridlridltkns ~/contracts/ ridl_token.wasm ridl_token.abi -p ridlridltkns@active

