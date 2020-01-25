#!/bin/bash

cleos -u https://nodes.get-scatter.com set contract ridlridltkns ../ridl_token -p ridlridltkns
cleos -u https://nodes.get-scatter.com set abi ridlridltkns ridl_token.abi -p ridlridltkns
