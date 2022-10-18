#pragma once

#include <eosio/asset.hpp>
#include <eosio/eosio.hpp>
#include <eosio/singleton.hpp>
#include <eosio/time.hpp>
#include <mdao.conf/mdao.conf.hpp>
#include "mdao.treasury.db.hpp"
#include <thirdparty/wasm_db.hpp>

using namespace eosio;
using namespace wasm::db;
using namespace mdao;
using namespace std;


enum class treasury_err: uint8_t {
    RECORD_NOT_FOUND        = 1,
    PERMISSION_DENIED       = 2,
    PARAM_ERROR             = 3,
    SYMBOL_ERROR            = 4,
    NOT_AVAILABLE           = 5,
    STATUS_ERROR            = 6,
    PLANS_EMPTY             = 7,
    INSUFFICIENT_VOTES      = 8,
    VOTED                   = 9,
    SYSTEM_ERROR            = 10,
    ACCOUNT_NOT_EXITS       = 11,
    STRATEGY_NOT_FOUND      = 12,
    VOTES_NOT_ENOUGH        = 13,
    ALREADY_EXISTS          = 14,
    ALREADY_EXPIRED         = 15,
    NOT_POSITIVE            = 16,
    INSUFFICIENT_BALANCE    = 17
};

class [[eosio::contract("treasury")]] mdaotreasury : public contract {

using conf_t = mdao::conf_global_t;
using conf_table_t = mdao::conf_global_singleton;

private:
    dbc                 _db;
    std::unique_ptr<conf_table_t> _conf_tbl_ptr;
    std::unique_ptr<conf_t> _conf_ptr;

    const conf_t& _conf();

public:
    using contract::contract;
    mdaotreasury(name receiver, name code, datastream<const char*> ds):_db(_self),  contract(receiver, code, ds){}

    [[eosio::on_notify("*::transfer")]] 
    void ontrantoken(name from, name to, asset quantity, string memo);

    // [[eosio::on_notify("amax.ntoken::transfer")]] 
    // void ontrannft( name from, name to, vector< nasset >& assets, string memo );

    [[eosio::action]] void tokentranout( name dao_code, name to, extended_asset quantity, string memo );
    // [[eosio::action]] void nfttranout( name from, name to, vector< nasset >& assets, string memo );
    
};