#pragma once

#include <eosio/eosio.hpp>
#include <eosio/asset.hpp>
#include <eosio/privileged.hpp>
#include <eosio/singleton.hpp>
#include <eosio/system.hpp>
#include <eosio/time.hpp>
#include "thirdparty/wasm_db.hpp"

using namespace eosio;
using namespace std;
using std::string;

using namespace wasm;

typedef double real_type;

#define SYMBOL(sym_code, precision) symbol(symbol_code(sym_code), precision)
static constexpr symbol SYS_SYMBOL = SYMBOL("AMAX", 8);
static constexpr name SYS_BANK = "amax.token"_n;
static constexpr name SYS_ACCT = "amax"_n;
static constexpr name MIRROR_BANK = "amax.mtoken"_n;

static constexpr symbol BRIDGE_SYMBOL = SYMBOL("BRIDGE", 4);
static constexpr uint64_t BRIDGE_AMOUNT = 1000000000000ll;

static constexpr name HOTPOT_BANK = "hotpotxtoken"_n;

static constexpr uint64_t RATIO_BOOST = 10000;
static constexpr uint64_t SLOPE_BOOST = 10000000000;
 static constexpr uint64_t MAX_CONTENT_SIZE = 64;

struct AppInfo_t {
    name app_name;
    string url;
    string logo;
    string summary;
};

struct Depository_t {
    name owner;
    string transmemo;
};

namespace transfer_type {
    static constexpr eosio::name create      = "create"_n;
    static constexpr eosio::name launch     = "launch"_n;
    static constexpr eosio::name bid         = "bid"_n;
    static constexpr eosio::name ask         = "ask"_n;
};

namespace admin_type {
    static constexpr eosio::name admin         = "admin"_n;
    static constexpr eosio::name feetaker      = "feetaker"_n;
    static constexpr eosio::name tokenarc      = "tokenarc"_n;
};

namespace market_status {
    static constexpr eosio::name created     = "created"_n;
    static constexpr eosio::name initialized     = "initialized"_n;
    static constexpr eosio::name trading         = "trading"_n;
    static constexpr eosio::name suspended       = "suspended"_n;
    static constexpr eosio::name delisted        = "delisted"_n;
};

namespace algo_type_t {
    static constexpr eosio::name bancor     = "bancor"_n;
    static constexpr eosio::name polycurve     = "polycurve"_n;
};

namespace algo_parma_type {
    static constexpr eosio::name aslope     = "aslope"_n;
    static constexpr eosio::name bvalue     = "bvalue"_n;
    static constexpr eosio::name cwvalue     = "cwvalue"_n;
    static constexpr eosio::name cwsupply     = "cwsupply"_n;
};

namespace wasm
{
namespace db
{
    #define HOTPOT_TBL [[eosio::table, eosio::contract("hotpotalgoex")]]
    #define HOTPOT_TBL_NAME(name) [[eosio::table(name), eosio::contract("hotpotalgoex")]]
    
    struct HOTPOT_TBL_NAME("global") global_t
    {
        AppInfo_t app_info;
        name exchg_status = market_status::created;
        map<name, name> admins;
        set<extended_symbol> quote_symbols;
        set<symbol_code> limited_symbols;
        asset token_crt_fee;
        uint16_t exchg_fee_ratio;

        EOSLIB_SERIALIZE(global_t, (app_info)(exchg_status)(admins)(quote_symbols)(limited_symbols)
            (token_crt_fee)(exchg_fee_ratio))
    };

    typedef eosio::singleton<"global"_n, global_t> global_singleton;

    struct HOTPOT_TBL market_t
    {
        symbol_code base_code;
        name creator;
        Depository_t launcher;
        name status;
        AppInfo_t app_info;
        name algo_type;
        map<name, uint64_t> algo_params;
        uint16_t in_tax;
        uint16_t out_tax;
        uint16_t parent_rwd_rate = 0;
        uint16_t grand_rwd_rate = 0;
        asset base_supply;
        asset quote_supply;
        extended_asset base_balance;
        extended_asset quote_balance;
        Depository_t taxtaker;
        time_point_sec created_at;

        uint64_t primary_key() const { return base_code.raw(); }

        market_t() {}
        market_t(const symbol_code &pbase_code) : base_code(pbase_code) {}

        asset convert_to_exchange_old( extended_asset& c, asset in );
        asset convert_from_exchange_old(extended_asset& c, asset in );
        asset convert_old( asset from, symbol to );
        
        asset convert_to_exchange(extended_asset& reserve, const asset& payment );
        asset convert_from_exchange(extended_asset& reserve, const asset& tokens );
        asset convert(const asset& from, const symbol& to);

        asset poly_to_exchange(const asset& in);
        asset poly_from_exchange(const asset& in);
    
        typedef wasm::db::multi_index<"algomkt"_n, market_t> idx_t;

        EOSLIB_SERIALIZE(market_t, (base_code)(creator)(launcher)(status)(app_info)
            (algo_type)(algo_params)(in_tax)(out_tax)(parent_rwd_rate)
            (grand_rwd_rate)
            (base_supply)(quote_supply)
            (base_balance)(quote_balance)(taxtaker)(created_at))
    };


    struct HOTPOT_TBL quotes_t
    {
        symbol_code base_code;
        symbol_code quote_symbol;
        uint64_t open;
        uint64_t high;
        uint64_t low;
        uint64_t close;
        uint64_t pre_close;
        uint64_t volume;
        uint64_t amount;
        time_point_sec updated_at;

        uint64_t primary_key() const { return base_code.raw(); }

        quotes_t() {}
        quotes_t(const symbol_code &pbase_code) : base_code(pbase_code) {}

        typedef wasm::db::multi_index<"quotes"_n, quotes_t> idx_t;

        EOSLIB_SERIALIZE(quotes_t, (base_code)(quote_symbol)(open)(high)
            (low)(close)(pre_close)(volume)(amount)
            (updated_at))
    };
}
}
