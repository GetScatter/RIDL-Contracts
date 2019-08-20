#!/bin/bash
shopt -s expand_aliases
source ~/.bash_aliases

name=${1:-test}
key=${2:-EOS6MRyAjQq8ud7hVNYcfnVPJqcVpscN5So8BhtHuGYqET5GDW5CV}

cleos system newaccount eosio --transfer "$name" "$key" "$key" --stake-net "20.0000 EOS" --stake-cpu "1000.0000 EOS" --buy-ram-kbytes 100000
