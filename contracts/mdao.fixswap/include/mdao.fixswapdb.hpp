#pragma once

#include <eosio/eosio.hpp>
#include <eosio/asset.hpp>
#include <eosio/privileged.hpp>
#include <eosio/singleton.hpp>
#include <eosio/system.hpp>
#include <eosio/time.hpp>
#include <thirdparty/wasm_db.hpp>
#include <amax.ntoken/amax.ntoken.db.hpp>

using namespace eosio;
using namespace std;
using namespace amax;

using std::string;

using namespace wasm;
#define SYMBOL(sym_code, precision) symbol(symbol_code(sym_code), precision)

static constexpr uint64_t seconds_per_day                   = 24 * 3600;
static constexpr uint64_t default_expired_secs              = 1 * seconds_per_day;
static constexpr uint64_t percent_boost                     = 10000;
// #define HASH256(str) sha256(str.c_str(), str.size())

static constexpr name   APLINK_FARM                         = "aplink.farm"_n;
static constexpr symbol   APLINK_SYMBOL                     = SYMBOL("APL", 4);

using checksum256 = fixed_bytes<32>;


namespace swap_status_t {
    static constexpr eosio::name created                    = "created"_n;
    static constexpr eosio::name initialized                = "initialized"_n;
    static constexpr eosio::name maintaining                = "maintaining"_n;

    static constexpr eosio::name matching                   = "matching"_n;
    static constexpr eosio::name cancel                     = "cancel"_n;
    static constexpr eosio::name matched                    = "matched"_n;
};

namespace wasm
{
    namespace db
    {

    #define SWAP_TBL [[eosio::table, eosio::contract("fixswap")]]
    #define SWAP_TBL_NAME(name) [[eosio::table(name), eosio::contract("fixswap")]]
    
    struct SWAP_TBL_NAME("global") gswap_t
    {
        name status                     = swap_status_t::created;
        name fee_collector;
        name admin;
        uint32_t make_fee_ratio         = 20;
        uint32_t take_fee_ratio         = 20;
        uint64_t swap_id                = 0;
        uint64_t farm_lease_id          = 0;
        set<name> supported_tokens;
        map <extended_symbol, int64_t> farm_scales;
        map <extended_symbol, int64_t> min_order_amount;

        EOSLIB_SERIALIZE(gswap_t, (status)(fee_collector)(admin)(make_fee_ratio)(take_fee_ratio)(swap_id)(farm_lease_id)(supported_tokens)(farm_scales)(farm_scales)(min_order_amount))
    };

    typedef eosio::singleton<"global"_n, gswap_t> gswap_singleton;

    struct SWAP_TBL swap_t
    {
        name orderno;
        uint64_t id;
        name status;
        name type;
        name maker;
        name taker = name(0);
        extended_asset make_asset;
        extended_asset take_asset;
        string code;
        time_point_sec  created_at;
        time_point_sec  expired_at;
       
        uint64_t primary_key() const { return orderno.value; }
        uint64_t by_maker()const { return maker.value; }

        swap_t() {}
        swap_t(const name &porderno) : orderno(porderno) {}

        typedef eosio::multi_index<"orders"_n, swap_t,
            indexed_by<"maker"_n, const_mem_fun<swap_t, uint64_t, &swap_t::by_maker> >
        > idx_t;

        EOSLIB_SERIALIZE(swap_t, (orderno)(id)(status)(type)(maker)(taker)(make_asset)(take_asset)(code)(created_at)(expired_at))
    };
}
}
