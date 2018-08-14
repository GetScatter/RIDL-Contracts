#pragma once

#include <eosiolib/eosio.hpp>
#include "../models/identity.h"
#include "../models/paid.h"
#include "../models/reputation.h"
#include "../../ridl_token/ridl_token.hpp"

using namespace eosio;
using namespace identity;
using namespace paid;
using namespace reputation;

class IdentityActions {
public:
    account_name __self;
    IdentityActions(account_name self):__self(self){}

    // Identifies a new user or re-registers an existing user
    void identify(string& username, public_key& key, account_name& account){
        require_auth(account);

        // Constructing new identity
        Identity identity = Identity(username, key, account);

        // Name is valid
        assertValidName(identity.username);

        PaidNames paidNames(__self, __self);
        auto paid = paidNames.find(identity.fingerprint);

        // Assertions
        eosio_assert(paid != paidNames.end(), "Name has not been paid for");
        eosio_assert(now() < paid->expires, "Name payment is expired.");
        eosio_assert(paid->account == account, "Identity payment does not belong to this account.");

        // Set Identity
        createOrUpdateIdentity(identity);

        // Release Payment
        releaseIdentityPayment(identity.fingerprint);

        string f = "user::"+username;
        uuid repFingerprint = toUUID(f);

        asset repTotal = RepTotal(__self, repFingerprint).get_or_default(asset(0'0000, string_to_symbol(4, "REP")));
        RepTotal(__self, repFingerprint).set(repTotal, account);

        // TODO: Deferred 1 year transaction to erase
        // TODO: Only allow repayment starting 4 months from end of payment year ( makes sure users can't pay for 10 years at once )
        // TODO: Allows users to repay their identity ( expires +1 year from previous ending date, not date of payment if renewed )
    }



    /***
     * Destroys an Identity and all it's owned assets.
     * @param fingerprint
     * @param account
     * @param sig
     */
    void release(uuid& fingerprint, account_name& account, signature& sig){
        require_auth(account);

        Identities identities(__self, __self);
        auto identity = identities.find(fingerprint);
        assertExists(identities, identity, account);

        // Because this is such a destructive action we will also make sure that
        // the user owns the Scatter identity's keypair as well,
        // and not just the keypair for the linked account.
        identity->prove(sig);

        identities.erase(identity);

        // TODO: Remove all reputation + reputation mines owned by user
        // Some reputation pots will be locked in the user's RAM until the next user comes along if they
        // haven't gotten rid of them yet. The only other way would be to release them to the contract itself
        // but that could become an attack vector.
    }



    /***
     * Changes the Identity's public key.
     * We are not using key signatures here because it might be that the user
     * has simply lost their Scatter instance and has no backup.
     * However - This can only be done with the primary account.
     * @param fingerprint
     * @param account
     * @param new_key
     */
    void rekey(uuid& fingerprint, account_name& account, public_key& new_key){
        require_auth(account);

        Identities identities(__self, __self);
        auto identity = identities.find(fingerprint);
        assertExists(identities, identity, account);

        identities.modify(identity, account, [&](auto& record){
            record.key = new_key;
        });
    }

    /***
     * Updates the account linked to an identity
     * Note - Because the original account could no longer be owned by the user they
     * can still change their primary account using the keypair on the identity
     * @param fingerprint
     * @param new_account
     * @param sig
     */
    void setaccount(uuid& fingerprint, account_name& new_account, signature& sig){
        Identities identities(__self, __self);
        auto identity = identities.find(fingerprint);
        eosio_assert(identity != identities.end(), "Identity does not exist");
        identity->prove(sig);

        identities.modify(identity, 0, [&](auto& record){
            record.account = new_account;
        });
    }



    // TODO: Bid on an existing identity
    void bid(string& username, account_name& bidder, asset& bid){}

    // TODO: Transfers an Identity ( Should be multi-sig so the recipient can claim using their identity's key )
    void sell(string& username, account_name& owner, uuid& bid_id){}





    /***
     * Seeds a reserved identity into the system
     * @param username
     * @param key
     */
    void seed(string& username, public_key& key){
        require_auth(__self);

        Identity identity = Identity(username, key, __self);
        identity.expires = now() + ((seconds_per_day * 365) * 100);
        createOrUpdateIdentity(identity, 100'0000);
    }

    /***
     * Claims a seeded identity
     * @param username
     * @param key - New identity key
     * @param sig - Signature from reservation linked key
     * @param account
     */
    void claim(string& username, public_key& key, signature& sig, account_name& account){
        require_auth(account);

        lower(username);
        uuid fingerprint = toUUID(username);

        Identities identities(__self, __self);
        auto identity = identities.find(fingerprint);
        eosio_assert(identity != identities.end(), "Identity not seeded.");
        eosio_assert(identity->account == __self, "Identity already owned.");

        // Proving ownership of public key
        identity->prove(sig);

        identities.modify(identity, account, [&](auto& record){
            record.account = account;
            record.key = key;
        });
    }

    void loadedtokens( const currency::transfer& t ){
        // Token Assertions
        eosio_assert(t.quantity.symbol == string_to_symbol(4, "RIDL"), "Token must be RIDL");
        eosio_assert(t.quantity.is_valid(), "Token asset is not valid");
        eosio_assert(t.quantity.amount > 0, "Not enough tokens");

        // Memo Assertions
        eosio_assert(t.memo.length() > 0, "Memo can not be empty");
        std::vector<string> params = memoToApiParams(t.memo);
        eosio_assert(params.size() == 2, "Memo must have 2 parameters");

        account_name account = string_to_name(params[0].c_str());
        string username = params[1].c_str();

        lower(username);
        uuid fingerprint = toUUID(username);
        Identities identities(__self, __self);
        auto identity = identities.find(fingerprint);
        eosio_assert(identity != identities.end(), "Cannot transfer RIDL tokens to an Identity that does not exist");

        asset quantity = t.quantity + identity->tokens;

        uint64_t reputationBonus = 100'0000 + RepTotal(__self, fingerprint).get_or_default(asset(0'0000, string_to_symbol(4, "REP"))).amount;
        asset maxCanHold = asset(reputationBonus, string_to_symbol(4, "RIDL"));

        // Too much was spent, sending the overage back
        if(quantity > maxCanHold){
            asset refunded = quantity - maxCanHold;
            quantity = quantity - refunded;
            INLINE_ACTION_SENDER(ridl::token, transfer)( N(ridlridlcoin), {{__self,N(active)}}, {__self, t.from, refunded, "RIDL Over-Loaded Refund"} );
        }

        eosio_assert(quantity.amount > 0, "Identity is already holding max RIDL");

        identities.modify(identity, 0, [&](auto& record){
            record.tokens = quantity;
        });
    }

    void paidForIdentity( const currency::transfer& t ){

        // TODO: Set as config var or get from oracle
        int64_t price = 1'0000;

        // Token Assertions
        eosio_assert(t.quantity.symbol == string_to_symbol(4, "EOS"), "Token must be EOS");
        eosio_assert(t.quantity.is_valid(), "Token asset is not valid");
        eosio_assert(t.quantity.amount >= price, "Not enough tokens");

        // Memo Assertions
        eosio_assert(t.memo.length() > 0, "Memo can not be empty");
        std::vector<string> params = memoToApiParams(t.memo);
        eosio_assert(params.size() == 2, "Memo must have 2 parameters");

        // Receiver account must exist on the blockchain
        account_name account = string_to_name(params[0].c_str());
        eosio_assert(is_account(account), "Receiving account does not exist");

        // Identity name formatting and assertions.
        lower(params[1]);
        assertValidName(params[1]);
        uuid fingerprint = toUUID(params[1]);

        // Name must be unique and unregistered
        Identities identities(__self, __self);
        auto previousIdentity = identities.find(fingerprint);
        eosio_assert(previousIdentity == identities.end() || now() > previousIdentity->expires, "Identity already exists and is not expired.");

        // Name must not be in pay system
        PaidNames paidNames(__self, __self);
        auto previousPayment = paidNames.find(fingerprint);
        eosio_assert(previousPayment == paidNames.end() || now() > previousPayment->expires, "Identity already paid for.");

        // Erasing old payments that were not cleared.
        if(previousPayment != paidNames.end()) paidNames.erase(previousPayment);

        paidNames.emplace(t.from, [&](auto& record){
            record = Paid(fingerprint, account, t.from);
        });
    }

private:
    /***
     * Removes the payment row once the account has identified themselves
     * and claimed their identity.
     * @param fingerprint
     */
    void releaseIdentityPayment(uuid& fingerprint){
        PaidNames paidNames(__self, __self);
        auto row = paidNames.find(fingerprint);
        if(row != paidNames.end()) paidNames.erase(row);
    }

    /***
     * Checks whether an identity exists, and whether a specified account owns it.
     * @param identities - initialized Identities multi_index
     * @param identity - Identities.find() result
     * @param account - The account to check for
     * @param primaryOnly - Check against the primary account only
     */
    void assertExists(Identities& identities, Identities::const_iterator& identity, account_name& account){
        eosio_assert(identity != identities.end(), "Identity does not exist");
        eosio_assert(identity->account == account, "Identity does not belong to specified account");

    }


    void createOrUpdateIdentity(Identity& identity, int64_t tokens = 20'0000){
        // Existence
        Identities identities(__self, __self);
        auto existingIdentity = identities.find(identity.fingerprint);
        eosio_assert(existingIdentity == identities.end() || now() > existingIdentity->expires, "Identity already registered.");


        // Registering new identity
        if(existingIdentity == identities.end()){
            identities.emplace(identity.account, [&](auto& record){
                record = identity;
                record.state = 0;

                record.tokens = asset(tokens, string_to_symbol(4, "RIDL"));
                // TODO: HANDLE RIDL RESERVES
//                INLINE_ACTION_SENDER(ridl::token, transfer)( N(ridlridlcoin), {{__self,N(active)}}, {N(ridlreserves), identity.account, record.tokens, "RIDL Over-Loaded Refund"} );

                if(identity.account != __self) {
                    INLINE_ACTION_SENDER(ridl::token, transfer)(N(ridlridlcoin), {{__self, N(active)}},
                                                                {__self, identity.account, record.tokens,
                                                                 "RIDL Over-Loaded Refund"});
                }
            });
        }

            // Updating expired identity
        else {
            //TODO: If identity is not the same account owner need to replace reputation.

            identities.modify(existingIdentity, identity.account, [&](auto& record){
                record = identity;
            });
        }
    }

};
