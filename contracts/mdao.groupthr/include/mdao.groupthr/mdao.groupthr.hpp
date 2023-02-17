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


enum class groupthr_err: uint8_t {
    PERMISSION_DENIED       = 1,
    SYSTEM_ERROR            = 2,
    ALREADY_EXPIRED         = 3,
    NOT_POSITIVE            = 4,
    QUANTITY_NOT_ENOUGH     = 5,
    SYMBOL_MISMATCH         = 6,
    NOT_AVAILABLE           = 7
};

namespace threshold_type {
    static constexpr eosio::name TOKEN_BALANCE      = "tokenbalance"_n;
    static constexpr eosio::name TOKEN_PAY          = "tokenpay"_n;
    static constexpr eosio::name NFT_BALANCE        = "nftbalance"_n;
    static constexpr eosio::name NFT_PAY            = "nftpay"_n;
};

static set<name> token_contracts = { {"amax.token"_n}};
static set<name> ntoken_contracts = { {"amax.ntoken"_n}};

class [[eosio::contract("mdao.groupthr")]] mdaogroupthr : public contract {

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

    void _create_groupthr( const name& dao_code,
                            const name& from,
                            const string_view& group_id,
                            const refasset& threshold,
                            const name& type );

    void _create_mermber( const name& from,
                            const uint64_t& groupthr_id );
public:
    using contract::contract;
    mdaogroupthr(name receiver, name code, datastream<const char*> ds):_db(_self),  contract(receiver, code, ds){}

    [[eosio::on_notify("*::transfer")]]
    void ontransfer();

    ACTION addbybalance(const name &mermber, const uint64_t &groupthr_id);

    ACTION setthreshold( const uint64_t &groupthr_id, const refasset &threshold, const name &type );

    ACTION delmermbers( vector<uint64_t> &mermbers );
};