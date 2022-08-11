
#pragma once

#include <eosio/asset.hpp>
#include <eosio/singleton.hpp>
#include <eosio/privileged.hpp>
#include <eosio/name.hpp>

#include <map>

using namespace eosio;


namespace xdao {

using namespace std;
using namespace eosio;
#define SYMBOL(sym_code, precision) symbol(symbol_code(sym_code), precision)
#define CONF_TG_TBL [[eosio::table, eosio::contract("xdao.conf")]]
static constexpr symbol AMAX_SYMBOL            = SYMBOL("AMAX", 8);

static constexpr name AMAX_TOKEN{"amax.token"_n};
static constexpr name XDAO_CONF{"xdao.conf"_n};
static constexpr name XDAO_STG{"xdao.stg"_n};
static constexpr name XDAO_GOV{"xdao.gov"_n};
static constexpr name AMAX_MULSIGN{"amax.mulsign"_n};

namespace conf_status {
    static constexpr name INITIAL    = "initial"_n;
    static constexpr name RUNNING    = "running"_n;
    static constexpr name MAINTAIN   = "maintain"_n;
    static constexpr name CANCEL     = "cancel"_n;

};

namespace manager {
    static constexpr name INFO       = "info"_n;
    static constexpr name STRATEGY   = "strategy"_n;
    static constexpr name WALLET     = "wallet"_n;
    static constexpr name TOKEN      = "token"_n;
    static constexpr name GOV        = "gov"_n;

};

struct app_info {
    string latest_app_version;
    string android_apk_download_url; //ipfs url
    string upgrade_log;
    bool force_upgrade;
    friend bool operator < (const app_info& symbol1, const app_info& appinfo2);
    EOSLIB_SERIALIZE(app_info, (latest_app_version)(android_apk_download_url)(upgrade_log)(force_upgrade) )
};

bool operator < (const app_info& appinfo1, const app_info& appinfo2) {
    return appinfo1.latest_app_version < appinfo2.latest_app_version;
}

struct [[eosio::table("global"), eosio::contract("xdao.conf")]] conf_global_t {
    app_info          appinfo;
    name              status = conf_status::INITIAL;
    name              fee_taker;
    asset             dao_upg_fee;
    uint16_t          amc_token_seats_max = 200;
    uint16_t          evm_token_seats_max = 200;
    uint16_t          dapp_seats_max = 200;

    map<name, name>   managers {
        { "info"_n, manager::INFO },
        { "strategy"_n,manager::STRATEGY },
        { "wallet"_n, manager::WALLET },
        { "token"_n, manager::TOKEN }
    };

    EOSLIB_SERIALIZE( conf_global_t, (appinfo)(status)(fee_taker)(dao_upg_fee)(amc_token_seats_max)(evm_token_seats_max)(dapp_seats_max)(managers) )
};



typedef eosio::singleton< "global"_n, conf_global_t > conf_global_singleton;

} //amax