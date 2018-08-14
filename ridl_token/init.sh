#!/bin/bash

~/setup_nodeos.sh
~/account.sh ridlridlcoin EOS6MRyAjQq8ud7hVNYcfnVPJqcVpscN5So8BhtHuGYqET5GDW5CV
~/account.sh ridlridlridl EOS6MRyAjQq8ud7hVNYcfnVPJqcVpscN5So8BhtHuGYqET5GDW5CV
~/account.sh scatterfunds EOS6MRyAjQq8ud7hVNYcfnVPJqcVpscN5So8BhtHuGYqET5GDW5CV
~/account.sh ridlreserves EOS6MRyAjQq8ud7hVNYcfnVPJqcVpscN5So8BhtHuGYqET5GDW5CV
~/account.sh test1account EOS6MRyAjQq8ud7hVNYcfnVPJqcVpscN5So8BhtHuGYqET5GDW5CV
./set.sh
cleos push action ridlridlcoin create '[]' -p ridlridlcoin

cleos transfer eosio test1account "10000.0000 EOS" "" -p eosio
