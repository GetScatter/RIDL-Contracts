#!/bin/bash

shopt -s expand_aliases
source ~/.bash_aliases


echo "Balances"
cleos get table ridlridlridl eosio balances
echo ""
echo ""

echo "Identities"
cleos get table ridlridlridl ridlridlridl ids
echo ""
echo ""

echo "-------------------"
