#pragma once

#include <eosio/asset.hpp>
#include <eosio/eosio.hpp>
#include <eosio/singleton.hpp>
#include <eosio/time.hpp>
#include <xdao.conf/xdao.conf.hpp>
#include <xdaostg/xdaostgdb.hpp>
#include <xdao.gov/xdao.gov.hpp>
#include "xdao.info.db.hpp"

using namespace eosio;
using namespace wasm::db;
using namespace xdao;
using namespace std;
static constexpr symbol AMAX_SYMBOL = symbol(symbol_code("AMAX"), 8);
static constexpr uint64_t INFO_PERMISSION_AGING = 24 * 3600;

namespace info_type {
    static constexpr name GROUP        = "group"_n;
    static constexpr name DAO          = "dao"_n;

};

namespace info_status {
    static constexpr name RUNNING       = "running"_n;
    static constexpr name BLOCK         = "block"_n;

};

enum class info_err: uint8_t {
    ACCOUNT_NOT_EXITS       = 0,
    RECORD_NOT_FOUND        = 1,
    STRATEGY_NOT_FOUND      = 2,
    NOT_REPEAT_RECEIVE      = 4,
    RECORD_EXITS            = 5,
    GOVERNANCE_NOT_FOUND    = 6,
    SIZE_TOO_MUCH           = 7,
    WALLET_NOT_FOUND        = 8,
    CODE_REPEAT             = 9,
    TITLE_REPEAT            = 10,
    PERMISSION_DENIED       = 11,
    CANNOT_ZERO             = 12,
    INVALID_FORMAT          = 13,
    INCORRECT_FEE           = 14,
    NOT_AVAILABLE           = 15,
    SYSTEM_ERROR            = 16
};

class [[eosio::contract("xdao.info")]] xdaoinfo : public contract {

using conf_t = xdao::conf_global_t;
using conf_table_t = xdao::conf_global_singleton;

private:
    dbc                 _db;
    std::unique_ptr<conf_table_t> _conf_tbl_ptr;
    std::unique_ptr<conf_t> _conf_ptr;

    const conf_t& _conf();

public:
    using contract::contract;
    xdaoinfo(name receiver, name code, datastream<const char*> ds):_db(_self),  contract(receiver, code, ds){}

    // [[eosio::action]]
    // void createdao(const name& owner,const name& code, const string& title,
    //                        const string& logo, const string& desc,
    //                        const set<string>& tags, const map<name, string>& links);

    [[eosio::action]]
    void upgradedao(name from, name to, asset quantity, string memo);

    [[eosio::action]]
    void updatedao(const name& owner, const name& code, const string& logo, 
                            const string& desc,const map<name, string>& links,
                            const extended_symbol& amctoken, const string& groupid);

    [[eosio::action]]
    void setstrategy(const name& owner, const name& code, const uint64_t& stgid);

    [[eosio::action]]
    void binddapps(const name& owner, const name& code, const set<app_info>& dapps);

    [[eosio::action]]
    void bindevmgov(const name& owner, const name& code, const string& evmgov);

    [[eosio::action]]
    void bindamcgov(const name& owner, const name& code, const uint64_t& govid);

    [[eosio::action]]
    void bindevmtoken(const name& owner, const name& code, const evm_symbol& evmtoken);

    [[eosio::action]]
    void bindamctoken(const name& owner, const name& code, const extended_symbol& amctoken);

    [[eosio::action]]
    void bindevmwal(const name& owner, const name& code, const string& evmwallet, const string& chain);

    [[eosio::action]]
    void bindamcwal(const name& owner, const name& code, const uint64_t& walletid);

    [[eosio::action]]
    void delamcparam(const name& owner, const name& code, set<extended_symbol> tokens);

    [[eosio::action]]
    void delevmparam(const name& owner, const name& code, set<evm_symbol> tokens);

    [[eosio::action]]
    void updatestatus(const name& code, const bool& isenable);

};