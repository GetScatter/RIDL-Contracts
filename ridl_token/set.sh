#!/bin/bash

eosio-cpp --abigen --contract=token ridl_token.cpp -o ridl_token.wasm
cleos set contract ridlridlcoin . ridl_token.wasm ridl_token.abi -p ridlridlcoin@active
