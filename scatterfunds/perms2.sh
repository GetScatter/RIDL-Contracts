#!/bin/bash

PKEY=EOS5bvzU2LHw2kBxfhDTEfp2AWGAg84d1c2FLBwEt7Hswwr4B8KMM

cleos -u https://nodes.get-scatter.com set account permission scatterfunds active \
'{"threshold": 1,"keys": [{"key": "'$PKEY'","weight": 1}],"accounts": [{"permission":{"actor":"scatterfunds","permission":"eosio.code"},"weight":1}]}' owner
