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
    static constexpr name AGREE     = "agree"_n;
    static constexpr name REJECT    = "reject"_n;

};

namespace plan_type {
    static constexpr name SINGLE        = "single"_n;
    static constexpr name MULTIPLE      = "multiple"_n;

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
    static constexpr eosio::name createtoken        = "createtoken"_n;
    static constexpr eosio::name issuetoken         = "issuetoken"_n;
    //gov
    static constexpr eosio::name setvotestg         = "setvotestg"_n;
    static constexpr eosio::name setproposestg      = "setproposestg"_n;
    static constexpr eosio::name setleastvote_data  = "setleastvote"_n;
    static constexpr eosio::name setreqratio        = "setreqratio"_n;
    static constexpr eosio::name setlocktime        = "setlocktime"_n;
    //im  
    static constexpr eosio::name setjoinstg         = "setjoinstg"_n;
    //treasury
    static constexpr eosio::name tokentranout       = "tokentranout"_n;

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

struct createtoken_data {
    name code;                    
    name owner; 
    uint16_t transfer_ratio; 
    string fullname; 
    asset maximum_supply; 
    string metadata; 
};

struct issuetoken_data {
    name code;                    
    name to; 
    asset quantity; 
    string memo; 
};

struct setvotestg_data {
    name dao_code;    
    name vote_strategy_code;                
    uint64_t vote_strategy_id; 
    uint32_t require_participation;
    uint32_t require_pass;
};

struct setproposestg_data {
    name dao_code;   
    name propose_strategy_code;                 
    uint64_t propose_strategy_id; 
};

struct setleastvote_data {
    name dao_code;                    
    uint32_t require_votes; 
};

struct setreqratio_data {
    name dao_code;                    
    uint32_t require_ratio; 
};

struct setlocktime_data {
    name dao_code;                    
    uint16_t limit_update_hours; 
};

struct tokentranout_data {
    name dao_code;                    
    name to;     
    extended_asset quantity;                    
    string memo; 
};

typedef std::variant<updatedao_data, bindtoken_data, binddapp_data, createtoken_data, issuetoken_data, 
                     setvotestg_data, setproposestg_data, setleastvote_data, setreqratio_data, setlocktime_data, tokentranout_data
                    > action_data_variant;

class [[eosio::contract("mdaopropose2")]] mdaoproposal : public contract {

using conf_t = mdao::conf_global_t;
using conf_table_t = mdao::conf_global_singleton;

private:
    dbc                 _db;
    std::unique_ptr<conf_table_t> _conf_tbl_ptr;
    std::unique_ptr<conf_t> _conf_ptr;

    const conf_t& _conf();

public:
    using contract::contract;
    mdaoproposal(name receiver, name code, datastream<const char*> ds):_db(_self),  contract(receiver, code, ds){}

    ACTION create(const name& dao_code, const name& creator, 
                    const string& proposal_name, const string& desc, 
                    const string& title, const uint64_t& vote_strategy_id, 
                    const uint64_t& propose_strategy_id, const name& type);

    ACTION cancel(const name& owner, const uint64_t& proposalid);

    ACTION addplan( const name& owner, const uint64_t& proposal_id, const string& title, const string& desc );

    ACTION startvote(const name& executor, const uint64_t& proposal_id);

    ACTION execute(const uint64_t& proposal_id);

    ACTION votefor(const name& voter, const uint64_t& proposal_id, const uint32_t plan_id, const bool direction);

    ACTION setaction(const name& owner, const uint64_t& proposalid, 
                        const name& action_name, const name& action_account, 
                        const std::vector<char>& packed_action_data);
    
    ACTION recycledb(uint32_t max_rows);

    ACTION deletegov(uint64_t id);
private:
    void _check_proposal_params(const action_data_variant& data_var,  const name& action_name, const name& action_account, const conf_t& conf);
    void _cal_votes(const name dao_code, const strategy_t& vote_strategy, const name voter, int64_t& value);
};