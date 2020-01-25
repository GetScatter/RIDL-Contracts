# RIDL Identity contracts v2

This contract no longer requires that users/identities have actual EOSIO accounts.
The actions can be sent using a `sender` account safely as the parameters of the repute transactions are hashed and signed on the client side
before being sent to a propagation API, then sent to chain and validated on the smart contract level.

This also allows the RIDL ID contract to be put onto many chains at once and used as multiple sources of truth for off-chain reputation calculations.
Reputations can be resolved and aggregated across many chains.