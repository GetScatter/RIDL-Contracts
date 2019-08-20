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

    // TODO: Add specific collateral bonds with asset transfers in IBC contracts

    void create(string& username, string& title, string& details, uint64_t duration, uint64_t starts_in_seconds, asset& limit, uuid& fixed_party){
        ///////////////////////////////////
        // Identity verification
        auto identity = findIdentity(username);
        eosio_assert(identity != identities.end(), "Identity does not exist");
        identity->authenticate();

        eosio_assert(limit.amount >= 1, "You must use at least 1.0000 REP to create a bond.");
        canUseBonds(identity, limit.amount);

        Bond bond = Bond::create(identity->id, title, details, duration, starts_in_seconds, limit);

        eosio_assert(bond.expires < identity->expires, "Bonds can not expire after your identity expires.");

        bond.id = bonds.available_primary_key();
        if(fixed_party > 0) bond.fixed_party = fixed_party;
        bond.fingerprint = toUUID(title+details+username+limit.to_string()+std::to_string(fixed_party));

        auto index = bonds.get_index<"fingerprint"_n>();
        eosio_assert(index.find(bond.fingerprint) == index.end(), "There is already an identical bond for this identity.");

        bonds.emplace(identity->account, [&](auto& row){ row = bond; });

        sendDeferred(_self, name("closebond"), bond.id, duration+1, bond.id);
    }

    void disputebond(string& username, uuid& bond_id, asset rep){
        eosio_assert(rep.amount > 0, "You must use at least 0.0001 REP to dispute a bond.");
        ///////////////////////////////////
        // Identity verification
        auto identity = findIdentity(username);
        eosio_assert(identity != identities.end(), "Identity does not exist");
        identity->authenticate();

        auto bond = bonds.find(bond_id);
        eosio_assert(bond != bonds.end(), ("There is no bond with the ID: "+std::to_string(bond_id)).c_str());
        eosio_assert(bond->closed == 0, "This bond has already closed.");
        eosio_assert(bond->start_time >= now(), "This bond has already expired.");
        eosio_assert(bond->expires >= now(), "This bond has already expired.");
        eosio_assert(bond->votes < bond->limit, "This bond has already reached max votes.");
        eosio_assert(bond->fixed_party == 0 || bond->fixed_party == identity->id, "You are not allowed to vote on this bond.");
        eosio_assert(bond->identity != identity->id, "You are not allowed to vote on your own bonds.");

        asset repNeeded = asset(bond->limit.amount - bond->votes.amount, S_REP);

        // Not using more than needed to close out bond.
        if(rep.amount > repNeeded.amount) rep.amount = repNeeded.amount;

        canUseBonds(identity, rep.amount);

        bonds.modify(bond, same_payer, [&](auto& row){ row.votes += rep; });
        identities.modify(identity, same_payer, [&](auto& row){ row.usable_rep -= rep; });

        // Closing out the bond if reached limit
        if(bond->votes.amount + rep.amount >= bond->votes.amount){
            closebond(bond_id);
            cancelDeferred(bond->fingerprint);
        }
    }

    void closebond(uuid& bond_id){
        auto bond = bonds.find(bond_id);
        eosio_assert(bond != bonds.end(), ("There is no bond with the ID: "+std::to_string(bond_id)).c_str());
        eosio_assert(bond->expires <= now() || bond->votes >= bond->limit, "This bond has not expired or reached its limit yet.");
        eosio_assert(bond->closed == 0, "This bond is already closed.");

        auto identity = identities.find(bond->identity);
        eosio_assert(identity != identities.end(), "Identity no longer exists");

        bonds.modify(bond, same_payer, [&](auto& row){
            row.closed = 1;
        });

        identities.modify(identity, same_payer, [&](auto& row){
            row.bonded -= bond->limit;

            if (bond->isBusted()) {
                row.usable_rep -= bond->limit;

                if(bond->hasFixedParty()){
                    auto party = identities.find(bond->fixed_party);
                    if(party != identities.end()){
                        identities.modify(party, same_payer, [&](auto& prow){
                            prow.usable_rep += bond->limit;
                        });
                    }
                }
            }
        });
    }

    void erasebond(uuid& bond_id){
        auto bond = bonds.find(bond_id);
        eosio_assert(bond != bonds.end(), ("There is no bond with the ID: "+std::to_string(bond_id)).c_str());
        eosio_assert(bond->closed == 1, "Bond must be closed first.");

        ///////////////////////////////////
        // Identity verification
        auto identity = identities.find(bond->identity);
        eosio_assert(identity != identities.end(), "Bond owning identity no longer exists");
        identity->authenticate();

        bonds.erase(bond);
    }

    void cancelbond(string& username, uuid& bond_id){
        ///////////////////////////////////
        // Identity verification
        auto identity = findIdentity(username);
        eosio_assert(identity != identities.end(), "Identity does not exist");
        identity->authenticate();

        auto bond = bonds.find(bond_id);
        eosio_assert(bond != bonds.end(), ("There is no bond with the ID: "+std::to_string(bond_id)).c_str());
        eosio_assert(bond->fixed_party == identity->id, "You are not the trustee of this bond.");

        auto bonder = identities.find(bond->identity);
        eosio_assert(bonder != identities.end(), "The bonded identity no longer exists");

        identities.modify(bonder, same_payer, [&](auto& row){
            row.bonded -= bond->limit;

            if(bond->votes.amount > 0){
                identities.modify(identity, same_payer, [&](auto& prow){
                    prow.usable_rep += bond->votes;
                });
            }
        });

        bonds.erase(bond);
    }

private:

    void canUseBonds(Identities::const_iterator& identity, uint64_t minimum){
        eosio_assert(identity->created+(SECONDS_PER_DAY*30) <= now(), "Identities can only vote on bonds 30 days after registration.");
        eosio_assert(identity->usable_rep.amount >= MIN_REQUIRED_REP_FOR_BONDS, ("You need at least "+std::to_string(MIN_REQUIRED_REP_FOR_BONDS)+" available REP to create a bond.").c_str());
        eosio_assert(identity->usable_rep.amount - identity->bonded.amount >= minimum,
                     ("You do not have enough unbonded REP to vote on this bond. Only "+
                      std::to_string(identity->usable_rep.amount - identity->bonded.amount)+
                      " left but "+std::to_string(minimum)+" is needed.").c_str());
    }

};
