#!/bin/bash

eosio-cpp --abigen --contract=ridlridlridl ridl_id.cpp -o ridl_id.wasm
cleos set contract ridlridlridl . ridl_id.wasm ridl_id.abi -p ridlridlridl@active