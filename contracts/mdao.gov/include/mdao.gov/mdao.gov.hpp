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

namespace plan_type {
    static constexpr name SINGLE        = "single"_n;
    static constexpr name MULTIPLE      = "multiple"_n;

};

namespace strategy_action_type {
     static constexpr name VOTE         = "vote"_n;
     static constexpr name PROPOSE      = "propose"_n;

};

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
    INSUFFICIENT_WEIGHT     =12,
    RATIO_LESS_THAN_ZERO    =13,
    TIME_LESS_THAN_ZERO     =14,
    NOT_MODIFY              =15,
    TYPE_ERROR              =16,
    PARAM_ERROR             =17

};

class [[eosio::contract("mdaogovtest1")]] mdaogov : public contract {

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

    [[eosio::action]]
    ACTION create(const name& dao_code, const uint64_t& propose_strategy_id, 
                    const uint64_t& vote_strategy_id, const uint32_t& require_participation, 
                    const uint32_t& require_pass );

    [[eosio::action]]
    ACTION setvotestg(const name& dao_code, const uint64_t& vote_strategy_id, 
                        const uint32_t& require_participation, const uint32_t& require_pass );
                        

    [[eosio::action]]
    ACTION setproposestg(const name& dao_code, const uint64_t& propose_strategy_id);

    [[eosio::action]]
    ACTION setlocktime(const name& dao_code, const uint16_t& lock_time);

    [[eosio::action]]
    ACTION setvotetime(const name& dao_code, const uint16_t& vote_time);
    
    [[eosio::action]]
    ACTION startpropose(const name& creator, const name& dao_code, const string& title,
                                 const string& proposal_name, const string& desc, 
                                 const name& plan_type);

private:
    void _cal_votes(const name dao_code, const strategy_t& vote_strategy, const name voter, int64_t& value);

};