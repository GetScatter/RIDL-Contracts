#!/bin/bash


custodian=${1:-"eosio"}
token_account=${2:-"ridlridlpaid"}
account=${3:-"ridlridlridl"}

PARAMS="{\"config\":{\"custodian\":\"$custodian\",\"token_account\":\"$token_account\"}}"
echo "$PARAMS"
cleos push action ridlridlridl setconfig "$PARAMS" -p $account $custodian
