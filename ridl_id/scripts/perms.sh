#!/bin/bash

shopt -s expand_aliases
source ~/.bash_aliases

PKEY=EOS6MRyAjQq8ud7hVNYcfnVPJqcVpscN5So8BhtHuGYqET5GDW5CV

cleos set account permission ridlridlridl active \
'{"threshold": 1,"keys": [{"key": "'$PKEY'","weight": 1}],"accounts": [{"permission":{"actor":"ridlridlridl","permission":"eosio.code"},"weight":1}]}' owner
