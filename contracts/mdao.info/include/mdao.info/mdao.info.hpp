#pragma once

#include <eosio/asset.hpp>
#include <eosio/eosio.hpp>
#include <eosio/singleton.hpp>
#include <eosio/time.hpp>
#include <mdao.conf/mdao.conf.hpp>
#include <mdao.stg/mdao.stgdb.hpp>
#include <mdao.gov/mdao.gov.hpp>
#include "mdao.info.db.hpp"

using namespace eosio;
using namespace wasm::db;
using namespace mdao;
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
    INVALID_FORMAT      = 1,
    INCORRECT_FEE       = 2,
    TITLE_REPEAT        = 3,
    CODE_REPEAT         = 4,
    RECORD_NOT_FOUND    = 5,
    PERMISSION_DENIED   = 6,
    NOT_AVAILABLE       = 7,
    PARAM_ERROR         = 8,
    SYMBOL_ERROR        = 9,
    RECORD_EXITS        = 10,
    SIZE_TOO_MUCH       = 11,
    STRATEGY_NOT_FOUND  = 12,
    CANNOT_ZERO         = 13,
    GOVERNANCE_NOT_FOUND= 14,
    SYSTEM_ERROR        = 15,
    NO_AUTH             = 16,
    NOT_POSITIVE        = 17,
    NOT_ALLOW           = 18
};

class [[eosio::contract("mdaoinfotest")]] mdaoinfo : public contract {

using conf_t = mdao::conf_global_t;
using conf_table_t = mdao::conf_global_singleton;

private:
    dbc                 _db;
    std::unique_ptr<conf_table_t> _conf_tbl_ptr;
    std::unique_ptr<conf_t> _conf_ptr;

    const conf_t& _conf();

public:
    using contract::contract;
    mdaoinfo(name receiver, name code, datastream<const char*> ds):_db(_self),  contract(receiver, code, ds){}

    [[eosio::on_notify("*::transfer")]]
    void upgradedao(name from, name to, asset quantity, string memo);

    [[eosio::action]]
    void updatedao(const name& owner, const name& code, const string& logo, 
                            const string& desc,const map<name, string>& links,
                            const string& symcode, string symcontract, const string& groupid);

    [[eosio::action]]
    void setstrategy(const name& owner, const name& code, const name& stgtype, const uint64_t& stgid);

    [[eosio::action]]
    void binddapps(const name& owner, const name& code, const set<app_info>& dapps);

    [[eosio::action]]
    void bindgov(const name& owner, const name& code, const uint64_t& govid);

    [[eosio::action]]
    void bindtoken(const name& owner, const name& code, const extended_symbol& token);

    [[eosio::action]]
    void bindwal(const name& owner, const name& code, const uint64_t& walletid);

    [[eosio::action]]
    void delparam(const name& owner, const name& code, vector<extended_symbol> tokens);

    [[eosio::action]]
    void updatestatus(const name& code, const bool& isenable);

    ACTION recycledb(uint32_t max_rows);

    [[eosio::action]]
    void createtoken(const name& code, const name& owner, const uint16_t& taketranratio, 
                    const uint16_t& takegasratio, const string& fullname, const asset& maximum_supply);
};