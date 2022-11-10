#pragma once

#include <eosio/asset.hpp>
#include <eosio/eosio.hpp>
#include <eosio/singleton.hpp>
#include <eosio/time.hpp>
#include <mdao.conf/mdao.conf.hpp>
#include "mdao.propose.db.hpp"
#include <thirdparty/wasm_db.hpp>
#include <mdao.stg/mdao.stg.hpp>
// #include <amax.ntoken/amax.ntoken.hpp>

using namespace eosio;
using namespace wasm::db;
using namespace mdao;
using namespace std;

namespace proposal_status {
    static constexpr name CREATED       = "created"_n;
    static constexpr name VOTING        = "voting"_n;
    static constexpr name EXECUTED      = "executed"_n;
    static constexpr name CANCELLED     = "cancelled"_n;
    static constexpr name EXPIRED       = "expired"_n;


};

namespace vote_direction {
    static constexpr name APPROVE     = "approve"_n;
    static constexpr name DENY        = "deny"_n;
    static constexpr name WAIVE       = "waive"_n;

};

// namespace plan_type {
//     static constexpr name SINGLE        = "single"_n;
//     static constexpr name MULTIPLE      = "multiple"_n;

// };
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

enum class proposal_err: uint8_t {
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
    CANNOT_ZERO             = 16,
    SIZE_TOO_MUCH           = 17,
    NOT_POSITIVE            = 18,
    NO_AUTH                 = 19,
    NOT_ALLOW               = 20,
    CODE_REPEAT             = 21,
    TOKEN_NOT_EXIST         = 22,
    NOT_MODIFY              = 23,
    TIME_LESS_THAN_ZERO     = 24,
    INSUFFICIENT_BALANCE    = 25
};

namespace proposal_action_type {
    //info
    static constexpr eosio::name updatedao          = "updatedao"_n;
    static constexpr eosio::name bindtoken          = "bindtoken"_n;
    static constexpr eosio::name binddapp           = "binddapp"_n;
    // static constexpr eosio::name createtoken        = "createtoken"_n;
    // static constexpr eosio::name issuetoken         = "issuetoken"_n;
    //gov
    static constexpr eosio::name setvotestg         = "setvotestg"_n;
    static constexpr eosio::name setproposestg      = "setproposestg"_n;
    static constexpr eosio::name setvotetime        = "setvotetime"_n;
    static constexpr eosio::name setlocktime        = "setlocktime"_n;
    //im  
    // static constexpr eosio::name setjoinstg         = "setjoinstg"_n;
    //treasury
    // static constexpr eosio::name tokentranout       = "tokentranout"_n;

};

struct updatedao_data {
    name owner; 
    name code; 
    string logo; 
    string desc; 
    map<name, string> links;
    string symcode; 
    string symcontract;  
    string groupid;
};

struct bindtoken_data {
    name owner;                    
    name code; 
    extended_symbol token; 
};

struct binddapp_data {
    name owner;                    
    name code;                    
    set<app_info> dapps;                    
};

// struct createtoken_data {
//     name code;                    
//     name owner; 
//     uint16_t transfer_ratio; 
//     string fullname; 
//     asset maximum_supply; 
//     string metadata; 
// };

// struct issuetoken_data {
//     name code;                    
//     name to; 
//     asset quantity; 
//     string memo; 
// };

struct setvotestg_data {
    name dao_code;    
    uint64_t vote_strategy_id; 
    uint32_t require_participation;
    uint32_t require_pass;
};

struct setproposestg_data {
    name dao_code;   
    uint64_t proposal_strategy_id; 
    EOSLIB_SERIALIZE( setproposestg_data, (dao_code)(proposal_strategy_id) )

};

struct setvotetime_data {
    name dao_code;   
    uint16_t voting_period; 
};

struct setlocktime_data {
    name dao_code;                    
    uint16_t update_interval; 
};

// struct tokentranout_data {
//     name dao_code;                    
//     name to;     
//     extended_asset quantity;                    
//     string memo; 
// };

struct setpropmodel_data {
    name dao_code;                    
    name propose_model;     
};

typedef std::variant<updatedao_data, bindtoken_data, binddapp_data, 
                     setvotestg_data, setproposestg_data, setlocktime_data, setvotetime_data> action_data_variant;

class [[eosio::contract("mdaopropose2")]] mdaoproposal : public contract {

using conf_t = mdao::conf_global_t;
using conf_table_t = mdao::conf_global_singleton;

private:
    dbc                 _db;
    std::unique_ptr<conf_table_t> _conf_tbl_ptr;
    std::unique_ptr<conf_t> _conf_ptr;
    prop_global_t               _gstate;
    propose_global_singleton    _global;
    const conf_t& _conf();

public:
    using contract::contract;
    mdaoproposal(name receiver, name code, datastream<const char*> ds):_db(_self),  contract(receiver, code, ds), _global(_self, _self.value) {
        if (_global.exists()) {
            _gstate = _global.get();

        } else {
            _gstate = prop_global_t{};
        }
    }
    ACTION create(const name& creator, const name& dao_code, const string& title, const string& desc);

    ACTION cancel(const name& owner, const uint64_t& proposalid);

    ACTION addplan( const name& owner, const uint64_t& proposal_id, const string& title, const string& desc );

    ACTION startvote(const name& executor, const uint64_t& proposal_id);

    ACTION execute(const uint64_t& proposal_id);

    ACTION votefor(const name& voter, const uint64_t& proposal_id,  const string& title, const name& vote);

    ACTION setaction(const name& owner, const uint64_t& proposalid, 
                        const name& action_name, const name& action_account, 
                        const action_data_variant& data, 
                        const string& title);
    
    ACTION recycledb(uint32_t max_rows);

    ACTION deletepropose(uint64_t id);
    ACTION deletevote(uint32_t id); 

private:
    void _check_proposal_params(const action_data_variant& data_var,  const name& action_name, const name& action_account, const name& proposal_dao_code, const conf_t& conf);
    void _cal_votes(const name dao_code, const strategy_t& vote_strategy, const name voter, int64_t& value, const uint32_t& lock_time) ;
};
