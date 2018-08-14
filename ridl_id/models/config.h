#pragma once

using std::vector;

namespace config {

    // @abi table configs
    struct Config {
        account_name        custodian;
        account_name        token_account;

        Config(){}
        Config(account_name _custodian){
            custodian = _custodian;
        }

        EOSLIB_SERIALIZE( Config, (custodian)(token_account) )
    };

    typedef eosio::singleton<N(configs), Config>   Configs;

}
