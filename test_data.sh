#!/bin/bash

# TEST KEY, NEVER USE ON MAINNETS
# 5KNNCwxjTeCvhz5tZdcphA1RCEvSduCDkmQSVKkZTQunSD9Jfxw
USER_KEY=EOS8YQzaYLxT17fWAPueQBxRjHehTQYZEvgPAWPPH4mAuwTJi3mPN

#cleos push action ridlridlridl clean '["eosio"]' -p ridlridlridl
#ridl_id/scripts/set_types.sh


# TEST DATA
#cleos transfer test1account ridlridlridl "1.0000 EOS" "" -p test1account
#cleos transfer test1account ridlridlridl "1.0000 SYS" "" -p test1account
#cleos push action ridlridlridl identify '["test1account", "HelloWorld", "'$USER_KEY'"]' -p test1account
#cleos push action ridlridlridl identify '["test2account", "HelloWorld2", "'$USER_KEY'"]' -p test2account
#cleos push action ridlridlridl repute '["HelloWorld", "app::fortnite", [{"type":"social", "fingerprint":3425667939, "up":"1.0000 RIDL", "down":"0.0000 RIDL"}], ""]' -p test1account
#
#cleos push action ridlridlridl forcetype '["leadership", "app::fortnite", "Born Leader", "Sheep"]' -p ridlridlridl
#cleos push action ridlridlridl repute '["HelloWorld", "app::fortnite", [{"type":"leadership", "up":"1.0000 RIDL", "down":"0.0000 RIDL"}], ""]' -p test1account

cleos push action ridlridlcoin transfer '["test2account", "ridlridlridl", "50.0000 RIDL", ""]' -p test2account
cleos push action ridlridlridl topup '["HelloWorld2", "50.0000 RIDL"]' -p test2account
