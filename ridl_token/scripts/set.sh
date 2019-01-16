#!/bin/bash

# eosio-cpp --abigen --contract=token ridl_token.cpp -o ridl_token.wasm
# cleos set contract ridlridlcoin . ridl_token.wasm ridl_token.abi -p ridlridlcoin@active

eosio-cpp --abigen --contract=token ./ridl_token/ridl_token.cpp -o ~/contracts/ridl_token.wasm
sed -i 's/1.1/1.0/g' ~/contracts/ridl_token.abi
cleos set contract ridlridlcoin ~/contracts/ ridl_token.wasm ridl_token.abi -p ridlridlcoin@active

