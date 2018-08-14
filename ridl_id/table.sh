#!/bin/bash

table=$1
scope=${2:-"ridlridlridl"}

cleos get table ridlridlridl "$scope" "$table"
