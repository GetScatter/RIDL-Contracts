#!/bin/bash

# TEST KEY, NEVER USE ON MAINNETS
# 5KNNCwxjTeCvhz5tZdcphA1RCEvSduCDkmQSVKkZTQunSD9Jfxw
USER_KEY=EOS8YQzaYLxT17fWAPueQBxRjHehTQYZEvgPAWPPH4mAuwTJi3mPN
ID=HelloWorld
FRAG='[{"type":"social", "fingerprint":3425667939, "up":"1.0000 RIDL", "down":"0.0000 RIDL"}]'
CHAIN="eos::cf057bbfb72640471fd910bcb67639c22df9f92470936cddc1ade0e2f2e7dc4f"

#cleos push action ridlridlridl clean '["eosio"]' -p ridlridlridl
#ridl_id/scripts/set_types.sh


# TEST DATA
#cleos transfer test1account ridlridlridl "1.0000 EOS" "" -p test1account
#cleos transfer test1account ridlridlridl "1.0000 SYS" "" -p test1account
#cleos push action ridlridlridl identify '["test1account", "HelloWorld", "'$USER_KEY'"]' -p test1account
#cleos push action ridlridlridl identify '["test2account", "HelloWorld2", "'$USER_KEY'"]' -p test2account

# REPUTES

##################################################################
#                                           username, id, entity, type, fragments, network, parent, details
##################################################################
#cleos push action ridlridlridl repute '["'$ID'", 0, "eosio.system", "acc", '"$FRAG"', "'$CHAIN'", 0, ""]' -p test1account
#cleos push action ridlridlridl repute '["'$ID'", 0, "updateauth", "act", [{"type":"dangerous", "fingerprint":118999305, "up":"0.0000 RIDL", "down":"1.0000 RIDL"}], "", 1, ""]' -p test1account
cleos push action ridlridlridl forcetype '["leadership", 1, "Born Leader", "Sheep"]' -p ridlridlridl

#cleos push action ridlridlridl repute '["HelloWorld", "acc::eosio.token", [{"type":"social", "fingerprint":3425667939, "up":"1.0000 RIDL", "down":"0.0000 RIDL"}], "eos::aca376f206b8fc25a6ed44dbdc66547c36c6c33e3a119ffbeaef943642f0e906", 0, ""]' -p test1account


#cleos push action ridlridlridl forcetyp`e '["leadership", "app::fortnite", "Born Leader", "Sheep"]' -p ridlridlridl
#cleos push action ridlridlridl repute '["HelloWorld", "app::fortnite", [{"type":"leadership", "up":"1.0000 RIDL", "down":"0.0000 RIDL"}], ""]' -p test1account

#cleos push action ridlridlcoin transfer '["test2account", "ridlridlridl", "50.0000 RIDL", ""]' -p test2account
#cleos push action ridlridlridl loadtokens '["HelloWorld3", "10.0000 RIDL"]' -p test2account
