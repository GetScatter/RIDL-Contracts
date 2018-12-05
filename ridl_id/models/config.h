#pragma once

using std::vector;

namespace config {

    struct Config {
        name        custodian;
        name        token_account;

        Config(){}
        Config(name _custodian){
            custodian = _custodian;
        }
    };

    typedef singleton<"configs"_n, Config>   Configs;

}
