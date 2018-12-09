#!/bin/bash

echo "-------------------"
echo "Configs"
cleos get table ridlridlridl ridlridlridl configs
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
cleos get table ridlridlridl 2116336695 reputations
echo "-------------------"