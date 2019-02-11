#!/bin/bash

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
cleos get table ridlridlridl ridlridlridl reputables
echo ""
echo ""

echo "Reputation"
cleos get table ridlridlridl 1 reputations

echo "Topups"
cleos get table ridlridlridl ridlridlridl topups
echo "-------------------"