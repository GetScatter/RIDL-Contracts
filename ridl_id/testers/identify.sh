#!/bin/bash

cleos transfer eosio ridlridlridl "1.0000 SYS" "" -p eosio
cleos push action ridlridlridl identify '["eosio", "hello", "EOS7w5aJCv5B7y3a6f4WCwPSvs6TpCAoRGnGpiLMsSWbmxaZdKigd"]' -p eosio
cleos get table ridlridlridl ridlridlridl ids
