
#pragma once

#include <eosio/asset.hpp>
#include <eosio/singleton.hpp>
#include <eosio/privileged.hpp>
#include <eosio/name.hpp>

#include <map>

using namespace eosio;


namespace mdao {

using namespace std;
using namespace eosio;
#define SYMBOL(sym_code, precision) symbol(symbol_code(sym_code), precision)
#define CONF_TG_TBL [[eosio::table, eosio::contract("mdaoconftest")]]
static constexpr symbol AMAX_SYMBOL            = SYMBOL("AMAX", 8);

static constexpr name AMAX_TOKEN{"amax.token"_n};
static constexpr name XDAO_INFO{"mdaoinfotest"_n};
static constexpr name XDAO_CONF{"mdaoconftest"_n};
static constexpr name XDAO_STG{"mdao.stg"_n};
static constexpr name XDAO_GOV{"mdao.gov"_n};
static constexpr name XDAO_WALLET{"mdao.wallet"_n};
static constexpr name XDAO_TOKEN{"mdaotoken111"_n};
static constexpr name XDAO_PROPOSE{"mdaopropose"_n};

static constexpr name AMAX_MULSIGN{"amax.mulsign"_n};

namespace conf_status {
    static constexpr name INITIAL    = "initial"_n;
    static constexpr name RUNNING    = "running"_n;
    static constexpr name MAINTAIN   = "maintain"_n;
    static constexpr name CANCEL     = "cancel"_n;

};

namespace manager_type {
    static constexpr name INFO       = "info"_n;
    static constexpr name STRATEGY   = "strategy"_n;
    static constexpr name WALLET     = "wallet"_n;
    static constexpr name TOKEN      = "token"_n;
    static constexpr name GOV        = "gov"_n;
    static constexpr name PROPOSE    = "propose"_n;

};

struct app_info {
    name app_name;
    string app_version;
    string url;
    string logo;
    friend bool operator < (const app_info& appinfo1, const app_info& appinfo2);
    EOSLIB_SERIALIZE(app_info, (app_name)(app_version)(url)(logo) )
};

bool operator < (const app_info& appinfo1, const app_info& appinfo2) {
    return appinfo1.app_name < appinfo2.app_name;
}

struct [[eosio::table("global"), eosio::contract("mdaoconftest")]] conf_global_t {
    app_info          appinfo;
    name              status = conf_status::INITIAL;
    name              fee_taker;
    asset             dao_upgrade_fee;
    uint16_t          token_seats_max = 200;
    uint16_t          dapp_seats_max = 200;
    name              dao_admin;
    set<symbol_code>  limited_symbols {
        symbol_code("USDT"), symbol_code("DOGE"), symbol_code("WBTC"),
        symbol_code("SHIB"), symbol_code("AVAX"), symbol_code("LINK"),
        symbol_code("MATIC"), symbol_code("NEAR"), symbol_code("ALGO"),
        symbol_code("ATOM"), symbol_code("MANA"), symbol_code("HBAR"),
        symbol_code("THETA"), symbol_code("TUSD"), symbol_code("EGLD"),
        symbol_code("SAND"), symbol_code("IOTA"), symbol_code("AAVE"),
        symbol_code("WAVES"), symbol_code("DASH"), symbol_code("CAKE"),
        symbol_code("SAFE"), symbol_code("NEXO"), symbol_code("CELO"),
        symbol_code("KAVA"), symbol_code("INCH"), symbol_code("QTUM"),
        symbol_code("IOST"), symbol_code("IOTX"), symbol_code("STORJ"),
        symbol_code("ANKR"), symbol_code("COMP"), symbol_code("GUSD"),
        symbol_code("HIVE"), symbol_code("SUSHI"), symbol_code("KEEP"),
        symbol_code("POWR"), symbol_code("ARDR"), symbol_code("CELR"),
        symbol_code("DENT"), symbol_code("DYDX"), symbol_code("STEEM"),
        symbol_code("POLYX"),symbol_code("NEST"), symbol_code("TRAC"),
        symbol_code("REEF"), symbol_code("STPT"), symbol_code("ALPHA"),
        symbol_code("BAND"), symbol_code("PERP"), symbol_code("POND"),
        symbol_code("AERGO"), symbol_code("TOMO"), symbol_code("BADGER"),
        symbol_code("LOOM"), symbol_code("ARPA"), symbol_code("SERO"),
        symbol_code("MONA"), symbol_code("LINA"), symbol_code("CTXC"),
        symbol_code("DATA"), symbol_code("IRIS"), symbol_code("FIRO"),
        symbol_code("YFII"), symbol_code("AKRO"), symbol_code("WNXM"),
        symbol_code("NULS"), symbol_code("QASH"), symbol_code("FRONT"),
        symbol_code("TIME"), symbol_code("WICC"), symbol_code("MUSDT"),
        symbol_code("MBTC"), symbol_code("MSOL"), symbol_code("MBNB"),
        symbol_code("MBUSD"), symbol_code("MUSDC"), symbol_code("METH"),
        symbol_code("METC")
    };//Token creation restrictions
    
    asset token_create_fee = asset(1'0000'0000, AMAX_SYMBOL);

    map<name, name>   managers {
        { manager_type::INFO, XDAO_INFO },
        { manager_type::STRATEGY, XDAO_STG },
        { manager_type::WALLET, XDAO_WALLET },
        { manager_type::TOKEN, XDAO_TOKEN },
        { manager_type::PROPOSE, XDAO_PROPOSE },
        { manager_type::GOV, XDAO_GOV },

    };

    EOSLIB_SERIALIZE( conf_global_t, (appinfo)(status)(fee_taker)(dao_upgrade_fee)(token_seats_max)(dapp_seats_max)(dao_admin)(limited_symbols)(token_create_fee)(managers) )
};



typedef eosio::singleton< "global"_n, conf_global_t > conf_global_singleton;

} //amax