#pragma once

#include <eosio/asset.hpp>
#include <eosio/eosio.hpp>
#include <eosio/singleton.hpp>
#include <eosio/time.hpp>
#include <mdao.conf/mdao.conf.hpp>
#include "mdao.groupthr.db.hpp"
#include <amax.token/amax.token.hpp>
#include <amax.ntoken/amax.ntoken.hpp>
#include <aplink.token/aplink.token.hpp>
#include <thirdparty/wasm_db.hpp>
#include <mdao.info/mdao.info.db.hpp>
#include <thirdparty/utils.hpp>
#include <thirdparty/contract_function.hpp>
#include <set>
using namespace eosio;
using namespace wasm::db;
using namespace mdao;
using namespace std;

#define GET_UNION_TYPE_ID(prefix, suffix) \
{ ( (uint128_t) prefix.value << 64 ) | ((uint128_t) suffix.value << 64) }

enum class groupthr_err: uint8_t {
    PERMISSION_DENIED       = 1,
    SYSTEM_ERROR            = 2,
    ALREADY_EXPIRED         = 3,
    NOT_POSITIVE            = 4,
    QUANTITY_NOT_ENOUGH     = 5,
    SYMBOL_MISMATCH         = 6,
    NOT_AVAILABLE           = 7,
    TYPE_ERROR              = 8,
    NOT_INITED              = 9,
    CLOSED                  = 10
};

namespace threshold_type {
    static constexpr eosio::name TOKEN_BALANCE      = "tokenbalance"_n;
    static constexpr eosio::name TOKEN_PAY          = "tokenpay"_n;
    static constexpr eosio::name NFT_BALANCE        = "nftbalance"_n;
    static constexpr eosio::name NFT_PAY            = "nftpay"_n;
};

namespace threshold_plan_type {
    static constexpr eosio::name MONTH              = "month"_n;
    static constexpr eosio::name QUARTER            = "quarter"_n;
    static constexpr eosio::name YEAR               = "year"_n;
};

namespace plan_union_threshold_type {
    using namespace threshold_type;
    using namespace threshold_plan_type;
    static constexpr uint128_t TOKEN_BALANCE_MONTH  = GET_UNION_TYPE_ID(TOKEN_BALANCE, MONTH);
    static constexpr uint128_t NFT_BALANCE_MONTH    = GET_UNION_TYPE_ID(NFT_BALANCE, MONTH);
    static constexpr uint128_t TOKEN_PAY_MONTH      = GET_UNION_TYPE_ID(TOKEN_PAY, MONTH);
    static constexpr uint128_t TOKEN_PAY_QUARTER    = GET_UNION_TYPE_ID(TOKEN_PAY, QUARTER);
    static constexpr uint128_t TOKEN_PAY_YEAR       = GET_UNION_TYPE_ID(TOKEN_PAY, YEAR);
    static constexpr uint128_t NFT_PAY_MONTH        = GET_UNION_TYPE_ID(NFT_PAY, MONTH);
    static constexpr uint128_t NFT_PAY_QUARTER      = GET_UNION_TYPE_ID(NFT_PAY, QUARTER);
    static constexpr uint128_t NFT_PAY_YEAR         = GET_UNION_TYPE_ID(NFT_PAY, YEAR);
};

namespace member_type {
    static constexpr eosio::name INIT               = "init"_n;
    static constexpr eosio::name CREATED            = "created"_n;
};

static constexpr uint8_t month                     = 1;
static constexpr uint8_t months_per_quarter        = 3;
static constexpr uint8_t months_per_year           = 12;

static constexpr uint64_t seconds_per_month         = 24 * 3600 * 31;
static constexpr uint64_t seconds_per_quarter       = 24 * 3600 * 31 * 3;
static constexpr uint64_t seconds_per_year          = 24 * 3600 * 31 * 12;
static set<name> token_contracts                    = { {"amax.token"_n}};
static set<name> ntoken_contracts                   = { {"amax.ntoken"_n}};
static asset crt_groupthr_fee                       = asset(100000000,AMAX_SYMBOL);
static asset join_member_fee                        = asset(100000,AMAX_SYMBOL);

class [[eosio::contract("mdaogroupthr")]] mdaogroupthr : public contract {

using conf_t = mdao::conf_global_t;
using conf_table_t = mdao::conf_global_singleton;

private:
    dbc                 _db;
    std::unique_ptr<conf_table_t> _conf_tbl_ptr;
    std::unique_ptr<conf_t> _conf_ptr;

    const conf_t& _conf();

    void _on_token_transfer( const name &from,
                                const name &to,
                                const asset &quantity,
                                const string &memo);

    void _on_ntoken_transfer( const name& from,
                                const name& to,
                                const std::vector<nasset>& assets,
                                const string& memo );

    void _create_groupthr( const name& from,
                            const string_view& group_id,
                            const refasset& threshold,
                            const name& type );

    void _join_expense_member( const name& from,
                                        const groupthr_t& groupthr,
                                        const name& plan_tpye,
                                        const refasset& quantity);

    void _join_balance_member( const name& from,
                                const uint64_t& groupthr_id);

    void _init_member( const name& from,
                        const uint64_t& groupthr_id);
public:
    using contract::contract;
    mdaogroupthr(name receiver, name code, datastream<const char*> ds):_db(_self),  contract(receiver, code, ds){}

    [[eosio::on_notify("*::transfer")]]
    void ontransfer();

    ACTION join(const name &member, const uint64_t &groupthr_id);

    ACTION setthreshold( const uint64_t &groupthr_id, const refasset &threshold,  const name &plan_type );

    ACTION enablegthr( const uint64_t &groupthr_id, const bool &enable_threshold);

    ACTION delmembers( vector<deleted_member> &deleted_members );


    // ACTION delgroup( uint64_t gid );
    // ACTION delmember( uint64_t mid )

};