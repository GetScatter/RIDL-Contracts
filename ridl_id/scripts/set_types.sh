#!/bin/bash

shopt -s expand_aliases
source ~/.bash_aliases

cleos push action ridlridlridl forcetype '["security", 0, "Secure", "Insecure"]' -p ridlridlridl
cleos push action ridlridlridl forcetype '["privacy", 0, "Discreet", "Indiscreet"]' -p ridlridlridl
cleos push action ridlridlridl forcetype '["scam", 0, "Trusted", "Scam"]' -p ridlridlridl
cleos push action ridlridlridl forcetype '["solvency", 0, "Solvent", "Insolvent"]' -p ridlridlridl
cleos push action ridlridlridl forcetype '["social", 0, "", ""]' -p ridlridlridl
cleos push action ridlridlridl forcetype '["dangerous", 0, "Safe", "Dangerous"]' -p ridlridlridl
cleos push action ridlridlridl forcetype '["accuracy", 0, "Accurate", "Inaccurate"]' -p ridlridlridl
cleos push action ridlridlridl forcetype '["rarity", 0, "Rare", "Common"]' -p ridlridlridl
cleos push action ridlridlridl forcetype '["fees", 0, "Low/No Fees", "High Fees"]' -p ridlridlridl
cleos push action ridlridlridl forcetype '["user experience", 0, "", ""]' -p ridlridlridl
cleos push action ridlridlridl forcetype '["code standards", 0, "Open Source", "Closed Source"]' -p ridlridlridl
