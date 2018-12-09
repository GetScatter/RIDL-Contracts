#!/bin/bash


USERNAME="hello"
ENTITY="app::get-scatter.com"
FRAGS="[{\"type\":\"security\", \"up\":\"1.0000 RIDL\", \"down\":\"0.0000 RIDL\"}]"

PARAMS="{\"username\":\"$USERNAME\", \"entity\":\"$ENTITY\", \"fragments\":$FRAGS, \"details\":\"\"}"
echo "$PARAMS"
cleos push action ridlridlridl repute "$PARAMS" -p ridlridlridl -p eosio
