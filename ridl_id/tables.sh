#!/bin/bash

shopt -s expand_aliases
source ~/.bash_aliases

echo "-------------------"
echo "RepTypes"
cleos get table ridlridlridl ridlridlridl reptypes
echo ""
echo ""

echo "Balances"
cleos get table ridlridlridl eosio balances
echo ""
echo ""

echo "Identities"
cleos get table ridlridlridl ridlridlridl ids
echo ""
echo ""

echo "Reputables"
cleos get table ridlridlridl ridlridlridl reputables -l 500
echo ""
echo ""

echo "Reputation"
cleos get table ridlridlridl 2 reputations

echo "Topups"
cleos get table ridlridlridl ridlridlridl topups

echo "Bonds"
cleos get table ridlridlridl ridlridlridl bonds
echo "-------------------"
