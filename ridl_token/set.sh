#!/bin/bash

eosiocpp -o ridl_token.wast ridl_token.cpp
cleos set contract ridlridlcoin ../ridl_token -p ridlridlcoin
cleos set abi ridlridlcoin ridl_token.abi -p ridlridlcoin
