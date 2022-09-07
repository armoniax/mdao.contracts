
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
#define CONF_TG_TBL [[eosio::table, eosio::contract("xdaoconftest")]]
static constexpr symbol AMAX_SYMBOL            = SYMBOL("AMAX", 8);

static constexpr name AMAX_TOKEN{"amax.token"_n};
static constexpr name XDAO_INFO{"xdaoinfotest"_n};
static constexpr name XDAO_CONF{"xdaoconftest"_n};
static constexpr name XDAO_STG{"xdao.stg"_n};
static constexpr name XDAO_GOV{"xdao.gov"_n};
static constexpr name XDAO_TOKEN{"xdaotoken111"_n};

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

struct [[eosio::table("global"), eosio::contract("xdaoconftest")]] conf_global_t {
    app_info          appinfo;
    name              status = conf_status::INITIAL;
    name              fee_taker;
    asset             dao_upg_fee;
    uint16_t          amc_token_seats_max = 200;
    uint16_t          evm_token_seats_max = 200;
    uint16_t          dapp_seats_max = 200;

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
    
    asset token_crt_fee = asset(1, AMAX_SYMBOL);

    map<name, name>   managers {
        { "info"_n, manager::INFO },
        { "strategy"_n,manager::STRATEGY },
        { "wallet"_n, manager::WALLET },
        { "token"_n, manager::TOKEN }
    };

    EOSLIB_SERIALIZE( conf_global_t, (appinfo)(status)(fee_taker)(dao_upg_fee)(amc_token_seats_max)(evm_token_seats_max)(dapp_seats_max)(limited_symbols)(token_crt_fee)(managers) )
};



typedef eosio::singleton< "global"_n, conf_global_t > conf_global_singleton;

} //amax