#pragma once

#include <thirdparty/wasm_db.hpp>
#include <eosio/eosio.hpp>
#include <eosio/asset.hpp>
#include <eosio/privileged.hpp>
#include <eosio/singleton.hpp>
#include <eosio/system.hpp>
#include <eosio/time.hpp>

using namespace eosio;
using namespace std;
using std::string;

using namespace wasm;

#define SYMBOL(sym_code, precision) symbol(symbol_code(sym_code), precision)
static constexpr name APL_BANK              = "aplink.token"_n;

static constexpr uint32_t MAX_CONTENT_SIZE = 64;
static constexpr uint32_t MAX_ALGO_SIZE = 256;


namespace wasm { namespace db {

#define STG_TABLE [[eosio::table, eosio::contract("mdaostrategy")]]
#define STG_TABLE_NAME(name) [[eosio::table(name), eosio::contract("mdaostrategy")]]

struct STG_TABLE_NAME("global") stg_global_t {
    name conf_contract;
    map<name,string> algos;
    EOSLIB_SERIALIZE( stg_global_t, (conf_contract)(algos) )
};
typedef eosio::singleton< "global"_n, stg_global_t > stg_singleton;

typedef std::variant<symbol_code, nsymbol> refsymbol;

namespace strategy_status {
    static constexpr eosio::name testing            = "testing"_n;
    static constexpr eosio::name tested             = "tested"_n;
    static constexpr eosio::name published          = "published"_n;
};

namespace strategy_type {
    static constexpr eosio::name TOKEN_BALANCE         = "tokenbalance"_n;
    static constexpr eosio::name TOKEN_STAKE           = "tokenstake"_n;
    static constexpr eosio::name TOKEN_SUM             = "tokensum"_n;
    static constexpr eosio::name NFT_BALANCE           = "nftbalance"_n;
    static constexpr eosio::name NFT_STAKE             = "nftstake"_n;
    static constexpr eosio::name NFT_PARENT_STAKE      = "nparentstake"_n;
    static constexpr eosio::name NFT_PARENT_BALANCE    = "nparentbalanc"_n;
};

struct STG_TABLE strategy_t {
    uint64_t        id;
    name            creator;
    name            status;
    name            type;
    string          stg_name;
    string          stg_algo;
    name            ref_contract;
    refsymbol       ref_sym;
    time_point_sec  created_at;

    strategy_t() {}
    strategy_t(const uint64_t& pid): id(pid) {}

    uint64_t primary_key() const { return id; }

     uint128_t by_creator() const { return (uint128_t)creator.value << 64 | (uint128_t)id; }

    typedef eosio::multi_index<"stglist"_n, strategy_t,
        indexed_by<"creatoridx"_n,  const_mem_fun<strategy_t, uint128_t, &strategy_t::by_creator> >
    > idx_t;

    EOSLIB_SERIALIZE( strategy_t, (id)(creator)(status)(type)(stg_name)
        (stg_algo)(ref_contract)(ref_sym)(created_at) )
};
};
}