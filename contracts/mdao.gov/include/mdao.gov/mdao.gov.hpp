#pragma once

#include <eosio/asset.hpp>
#include <eosio/eosio.hpp>
#include <eosio/singleton.hpp>
#include <eosio/time.hpp>
#include "mdao.gov.db.hpp"
#include <mdao.conf/mdao.conf.db.hpp>

using namespace eosio;
using namespace wasm::db;
using namespace mdao;
using namespace std;

static constexpr symbol   AM_SYMBOL = symbol(symbol_code("AMAX"), 8);
static constexpr uint64_t PROPOSE_STG_PERMISSION_AGING = 24 * 3600;

// static constexpr name AMAX_TOKEN{"amax.token"_n};
// static constexpr name MDAO_CONF{"mdao.conf"_n};
// static constexpr name MDAO_STG{"mdao.stg"_n};
// static constexpr name MDAO_GOV{"mdao.gov"_n};
// static constexpr name MDAO_VOTE{"mdao.vote"_n};
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
    ACCOUNT_NOT_EXITS       =1,
    MIN_VOTES_LESS_THAN_ZERO=2,
    CODE_REPEAT             =3,
    NOT_AVAILABLE           =4,
    STRATEGY_NOT_FOUND      =5,
    RECORD_NOT_FOUND        =6,
    PERMISSION_DENIED       =7,
    TOO_FEW_VOTES           =8,
    PROPOSER_NOT_FOUND      =9,
    VOTE_STRATEGY_NOT_FOUND =10,
    SYSTEM_ERROR            =11,
    INSUFFICIENT_WEIGHT     =12
};

class [[eosio::contract("mdao.gov")]] mdaogov : public contract {

using conf_t = mdao::conf_global_t;
using conf_table_t = mdao::conf_global_singleton;

private:
    dbc                 _db;
    std::unique_ptr<conf_table_t> _conf_tbl_ptr;
    std::unique_ptr<conf_t> _conf_ptr;

    const conf_t& _conf();

public:
    using contract::contract;
    mdaogov(name receiver, name code, datastream<const char*> ds):_db(_self),  contract(receiver, code, ds){}

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