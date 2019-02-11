#pragma once

#include <eosiolib/eosio.hpp>
#include "../models/bond.h"
#include "../models/identity.h"

using namespace eosio;
using namespace bond;
using namespace identity;

using std::string;
using std::vector;
using std::make_tuple;

class BondActions {
private:
    name _self;
    Bonds bonds;
    Identities identities;

    auto findIdentity(string& username){
        lower(username);
        auto index = identities.get_index<"name"_n>();
        auto found = index.find(toUUID(username));
        eosio_assert(found != index.end(), ("Could not find username: "+username).c_str());
        return identities.find(found->id);
    }

public:
    BondActions(name self):_self(self),
        bonds(_self, _self.value),
        identities(_self, _self.value){}

    void create(string& username, string& title, string& details, uint64_t duration, asset& limit){
        ///////////////////////////////////
        // Identity verification
        auto identity = findIdentity(username);
        eosio_assert(identity != identities.end(), "Identity does not exist");
        identity->authenticate();

        eosio_assert(identity->total_rep.amount >= 500, "You need at least 500 available REP to create a bond.");
        eosio_assert(identity->total_rep.amount - identity->bonded.amount >= limit.amount,
                ("You do not have enough unbonded REP to create this bond. Only "+
                std::to_string(identity->total_rep.amount - identity->bonded.amount)+
                " left.").c_str());

        bonds.emplace(identity->account, [&](auto& row){
           row = Bond::create(identity->id, title, details, duration, limit);
           row.id = bonds.available_primary_key();
        });
    }

    void votebond(string& username, uuid& bond_id, asset rep){
        ///////////////////////////////////
        // Identity verification
        auto identity = findIdentity(username);
        eosio_assert(identity != identities.end(), "Identity does not exist");
        identity->authenticate();

        auto bond = bonds.find(bond_id);
        eosio_assert(bond != bonds.end(), ("There is no bond with the ID: "+std::to_string(bond_id)).c_str());
        eosio_assert(bond->expires >= now(), "This bond has already expired.");
        eosio_assert(bond->votes >= bond->limit, "This bond has already been closed out.");
        asset repNeeded = asset(bond->limit.amount - bond->votes.amount, S_REP);

        // Not using more than needed to close out bond.
        if(rep.amount > repNeeded.amount) rep.amount = repNeeded.amount;

        eosio_assert(identity->created+(SECONDS_PER_DAY*30) <= now(), "Identities can only vote on bonds 30 days after registration.");
        eosio_assert(identity->total_rep.amount - identity->bonded.amount >= rep.amount,
                     ("You do not have enough unbonded REP to vote on this bond. Only "+
                      std::to_string(identity->total_rep.amount - identity->bonded.amount)+
                      " left but "+std::to_string(rep.amount)+" is needed.").c_str());

        bonds.modify(bond, same_payer, [&](auto& row){ row.votes += rep; });
        identities.modify(identity, same_payer, [&](auto& row){ row.total_rep -= rep; });

        // Closing out the bond if reached limit
        if(bond->votes.amount + rep.amount >= bond->votes.amount){
            closebond(bond_id);
        }
    }

    void closebond(uuid& bond_id){
        auto bond = bonds.find(bond_id);
        eosio_assert(bond != bonds.end(), ("There is no bond with the ID: "+std::to_string(bond_id)).c_str());
        eosio_assert(bond->expires < now() && bond->votes < bond->limit, "This bond has not expired or reached its limit yet.");

        auto identity = identities.find(bond->identity);
        eosio_assert(identity != identities.end(), "Identity no longer exists");

        identities.modify(identity, same_payer, [&](auto& row){
            row.bonded -= bond->limit;

            // Removing REP that is now lost forever.
            if (bond->votes.amount >= bond->limit.amount){
                row.total_rep -= bond->limit;
            }
        });

        bonds.erase(bond);
    }

private:



};
