#!/bin/bash

name=${1:-test}
key=${2:-EOS6MRyAjQq8ud7hVNYcfnVPJqcVpscN5So8BhtHuGYqET5GDW5CV}

cleos create account eosio "$name" "$key"

# cleos system newaccount eosio --transfer "$name" "$key" --stake-net "1000.0000 SYS" --stake-cpu "1000.0000 SYS" --buy-ram-kbytes 100000
