
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
static constexpr symbol AMAX_SYMBOL            = SYMBOL("AMAX", 8);
static constexpr uint16_t TEN_THOUSAND         = 10000;

#define CONF_TG_TBL [[eosio::table, eosio::contract("mdao.conf")]]
#define CONF_TABLE_NAME(name) [[eosio::table(name), eosio::contract("mdao.conf")]]

static constexpr name AMAX_TOKEN{"amax.token"_n};
static constexpr name MDAO_INFO{"mdao.info"_n};
static constexpr name MDAO_CONF{"mdao.conf"_n};
static constexpr name MDAO_STG{"mdao.stg"_n};
static constexpr name MDAO_GOV{"mdao.gov"_n};
static constexpr name MDAO_PROPOSAL{"mdao.propose"_n};
static constexpr name MDAO_STAKE{"mdao.stake"_n};

namespace conf_status {
    static constexpr name INITIAL    = "initial"_n;
    static constexpr name RUNNING    = "running"_n;
    static constexpr name PENDING    = "pending"_n;
    static constexpr name CANCEL     = "cancel"_n;

};

namespace manager_type {
    static constexpr name INFO       = "info"_n;
    static constexpr name STRATEGY   = "strategy"_n;
    static constexpr name TREASURY   = "treasury"_n;
    static constexpr name TOKEN      = "token"_n;
    static constexpr name GOV        = "gov"_n;
    static constexpr name PROPOSAL   = "proposal"_n;
    static constexpr name CONF       = "conf"_n;
    static constexpr name ALGOEX     = "algoex"_n;
    static constexpr name STAKE      = "stake"_n;
    static constexpr name FACTORY    = "factory"_n;

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

struct CONF_TABLE_NAME("global") conf_global_t {
    app_info          appinfo;
    name              status = conf_status::INITIAL;
    name              fee_taker;
    asset             upgrade_fee;
    uint16_t          dapp_seats_max = 5;
    name              admin;
    //Token creation restrictions
    asset token_create_fee = asset(1'0000'0000, AMAX_SYMBOL);
    map<name, name>   managers {
        { manager_type::INFO, MDAO_INFO }
        // { manager_type::STRATEGY, MDAO_STG },
        // { manager_type::TOKEN, MDAO_TOKEN },
        // { manager_type::PROPOSAL, MDAO_PROPOSAL },
        // { manager_type::GOV, MDAO_GOV },
        // { manager_type::STAKE, MDAO_STAKE },
    };
    set<name> token_contracts;
    set<name> ntoken_contracts;
    uint16_t  stake_period_days = 2;
    set<symbol_code>  black_symbols {
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
        symbol_code("METC"), symbol_code("AMAX"), symbol_code("APLINK")
    };
    set<string>  available_tags {
        "a.recommend","a.hot","a.vip",
        "t.defi","t.socialfi","t.gamefi","t.nft","t.token",
        "t.tech","t.tour","t.sport","t.charity","t.film",
        "l.cn","l.en","l.vi","l.th","l.id","l.ko","l.ja","l.ru"
    };

    bool      enable_metaverse  = false;
    EOSLIB_SERIALIZE( conf_global_t,    (appinfo)(status)(fee_taker)(upgrade_fee)(dapp_seats_max)
                                        (admin)(token_create_fee)(managers)(token_contracts)(ntoken_contracts)
                                        (stake_period_days)(black_symbols)(available_tags)(enable_metaverse) )
};

typedef eosio::singleton< "global"_n, conf_global_t > conf_global_singleton;

} //amax