#pragma once

#include <eosio/asset.hpp>
#include <eosio/eosio.hpp>
#include <eosio/singleton.hpp>
#include <eosio/time.hpp>
#include "xdao.gov.db.hpp"
#include <xdao.conf/xdao.conf.db.hpp>

using namespace eosio;
using namespace wasm::db;
using namespace xdao;
using namespace std;

static constexpr symbol   AM_SYMBOL = symbol(symbol_code("AMAX"), 8);
static constexpr uint64_t PROPOSE_STG_PERMISSION_AGING = 24 * 3600;

// static constexpr name AMAX_TOKEN{"amax.token"_n};
// static constexpr name XDAO_CONF{"xdao.conf"_n};
// static constexpr name XDAO_STG{"xdao.stg"_n};
// static constexpr name XDAO_GOV{"xdao.gov"_n};
// static constexpr name XDAO_VOTE{"xdao.vote"_n};
// static constexpr name AMAX_MULSIGN{"amax.mulsign"_n};

namespace gov_status {
    static constexpr name RUNNING       = "running"_n;
    static constexpr name BLOCK         = "block"_n;    
    static constexpr name CANCEL        = "cancel"_n;
};

// namespace conf_status {
//     static constexpr name INITIAL    = "initial"_n;
//     static constexpr name RUNNING    = "running"_n;
//     static constexpr name CANCEL     = "cancel"_n;

// };

// namespace manager {
//     static constexpr name INFO       = "info"_n;
//     static constexpr name STRATEGY   = "strategy"_n;
//     static constexpr name GOV        = "gov"_n;
//     static constexpr name WALLET     = "wallet"_n;
//     static constexpr name TOKEN      = "token"_n;
//     static constexpr name VOTE       = "vote"_n;

// };

enum class gov_err: uint8_t {
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
    SYSTEM_ERROR            = 16,
    PROPOSER_NOT_FOUND      = 17,
    VOTE_STRATEGY_NOT_FOUND = 18,
    MIN_VOTES_LESS_THAN_ZERO= 19,
    TOO_FEW_VOTES           = 20
};

class [[eosio::contract("xdao.gov")]] xdaogov : public contract {

using conf_t = xdao::conf_global_t;
using conf_table_t = xdao::conf_global_singleton;

private:
    dbc                 _db;
    std::unique_ptr<conf_table_t> _conf_tbl_ptr;
    std::unique_ptr<conf_t> _conf_ptr;

    const conf_t& _conf();

public:
    using contract::contract;
    xdaogov(name receiver, name code, datastream<const char*> ds):_db(_self),  contract(receiver, code, ds){}

    // [[eosio::action]]
    // void createdao(const name& owner,const name& code, const string& title,
    //                        const string& logo, const string& desc,
    //                        const set<string>& tags, const map<name, string>& links);

    [[eosio::action]]
    ACTION creategov(const name& creator,const name& daocode, const string& title,
                            const string& desc, const set<uint64_t>& votestg, const uint32_t minvotes);

    [[eosio::action]]
    ACTION cancel(const name& owner, const name& daocode);

    [[eosio::action]]
    ACTION setpropstg( const name& owner, const name& daocode,
                            set<name> proposers, set<uint64_t> proposestg );

    [[eosio::action]]
    ACTION createprop(const name& owner, const name& creator, const name& daocode,
                                const uint64_t& stgid, const string& name, 
                                const string& desc, const uint32_t& votes);

    [[eosio::action]]
    ACTION excuteprop(const name& owner, const name& code, const uint64_t& proposeid);

};