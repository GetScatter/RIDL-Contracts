#!/bin/bash

cleos push action ridlridlridl clean '["eosio"]' -p ridlridlridl
./set_types.sh
./seed.sh
