#!/bin/bash

NAME=${1:-HelloWorld}
KEY=${2:-EOS6MRyAjQq8ud7hVNYcfnVPJqcVpscN5So8BhtHuGYqET5GDW5CV}

PARAMS="[\"$NAME\", \"$KEY\"]"

cleos push action ridlridlridl seed "$PARAMS" -p ridlridlridl
