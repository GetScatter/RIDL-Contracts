#!/bin/bash

shopt -s expand_aliases
source ~/.bash_aliases

PUBLIC_KEY=${1:-EOS6MRyAjQq8ud7hVNYcfnVPJqcVpscN5So8BhtHuGYqET5GDW5CV}

cleos push action ridlridlridl seed '["optimusprime", "'$PUBLIC_KEY'"]' -p ridlridlridl
