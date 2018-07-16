#!/bin/bash

PKEY=EOS6MRyAjQq8ud7hVNYcfnVPJqcVpscN5So8BhtHuGYqET5GDW5CV

cleos set account permission scatterfunds active \
'{"threshold": 1,"keys": [{"key": "'$PKEY'","weight": 1}],"accounts": [{"permission":{"actor":"scatterfunds","permission":"eosio.code"},"weight":1}]}' owner
