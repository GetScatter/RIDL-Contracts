#!/bin/bash

#comptract.sh scatterfunds scatterfunds
cleos set contract scatterfunds ../scatterfunds -p scatterfunds
cleos set abi scatterfunds scatterfunds.abi -p scatterfunds
./perms.sh
#cleos push action scatterfunds start '[]' -p scatterfunds

