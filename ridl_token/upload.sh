#!/bin/bash

cleos -u https://nodes.get-scatter.com set contract ridlridlcoin ../ridl_token -p ridlridlcoin
cleos -u https://nodes.get-scatter.com set abi ridlridlcoin ridl_token.abi -p ridlridlcoin
