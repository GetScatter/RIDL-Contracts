#!/bin/bash


PARAMS="{\"config\":{\"hash\":\"E3D6BC4D0E239D22C9D5CA78254519A533973072757B7EBFF7AD8B98FE70F4FF\"}}"

#custodian=${1:-"eosio"}
#token_account=${2:-"ridlridlpaid"}
#account=${3:-"ridlridlridl"}

#PARAMS="{\"config\":{\"custodian\":\"$custodian\",\"token_account\":\"$token_account\"}}"
echo "$PARAMS"
cleos push action ridlridlridl setconfig "$PARAMS" -p ridlridlridl
