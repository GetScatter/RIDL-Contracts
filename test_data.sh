#!/bin/bash

# TEST KEY, NEVER USE ON MAINNETS
# 5KNNCwxjTeCvhz5tZdcphA1RCEvSduCDkmQSVKkZTQunSD9Jfxw
USER_KEY=EOS8YQzaYLxT17fWAPueQBxRjHehTQYZEvgPAWPPH4mAuwTJi3mPN
ID=HelloWorld

cleos push action ridlridlridl clean '["eosio"]' -p ridlridlridl
cleos push action ridlridlridl setcreator '["eosio"]' -p ridlridlridl
cleos push action ridlridlridl identify '["HelloWorld", "'$USER_KEY'"]' -p eosio
cleos push action ridlridlridl identify '["HelloWorld2", "'$USER_KEY'"]' -p eosio
# cleos push action ridlridlridl identify '["HelloWorld2", "EOS8YQzaYLxT17fWAPueQBxRjHehTQYZEvgPAWPPH4mAuwTJi3mPN"]' -p eosio

# REPUTES

##################################################################
#                                           username, id, entity, type, fragments, network, parent, details
##################################################################
cleos push action ridlridlridl repute '[1, "eosio.system::updateauth", [{"type":"dangerous", "value":-1}, {"type":"security", "value":1}], "2.0000 RIDL", "Test memo"]' -p test1account
cleos push action ridlridlridl repute '[1, "eosio.system::updateauth", [{"type":"dangerous", "value":1}, {"type":"security", "value":1}], "2.0000 RIDL", "Test memo"]' -p test1account
cleos push action ridlridlridl repute '[1, "eosio.system::updateauth", [{"type":"dangerous", "value":-1}, {"type":"security", "value":-1}], "4.0000 RIDL", "Test memo"]' -p test1account
#cleos push action ridlridlridl repute '["HelloWorld", "updateauth", [{"type":"dangerous", "value":-1}, {"type":"security", "value":1}]]' -p test1account

#cleos push action ridlridltkns transfer '["test2account", "ridlridlridl", "50.0000 RIDL", ""]' -p test2account
#cleos push action ridlridlridl loadtokens '["HelloWorld3", "10.0000 RIDL"]' -p test2account

./ridl_id/tables.sh
