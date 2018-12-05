#!/bin/bash

name=${1:-test}
key=${2:-EOS6MRyAjQq8ud7hVNYcfnVPJqcVpscN5So8BhtHuGYqET5GDW5CV}

cleos create account eosio "$name" "$key"
