#pragma once

#include <eosio/asset.hpp>
#include <eosio/eosio.hpp>
#include <eosio/singleton.hpp>
#include <eosio/time.hpp>
#include <mdao.conf/mdao.conf.hpp>
#include "mdao.propose.db.hpp"
#include <thirdparty/wasm_db.hpp>

using namespace eosio;
using namespace wasm::db;
using namespace mdao;
using namespace std;

namespace propose_status {
    static constexpr name CREATED    = "create"_n;
    static constexpr name RUNNING    = "running"_n;
    static constexpr name EXCUTING   = "excuting"_n;
    static constexpr name CANCELLED  = "cancelled"_n;

};

namespace vote_type {
    static constexpr name PLEDGE     = "pledge"_n;
    static constexpr name TOKEN      = "token"_n;

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

enum class propose_err: uint8_t {
    RECORD_NOT_FOUND        = 1,
    PERMISSION_DENIED       = 2,
    PARAM_ERROR             = 3,
    SYMBOL_ERROR            = 4,
    NOT_AVAILABLE           = 5,
    STATUS_ERROR            = 6,
    OPTS_EMPTY              = 7,
    INSUFFICIENT_VOTES      = 8,
    VOTED                   = 9,
    SYSTEM_ERROR            = 10,
    ACCOUNT_NOT_EXITS       = 11,
    STRATEGY_NOT_FOUND      = 12

};

namespace proposal_action_type {
    static constexpr eosio::name updatedao          = "updatedao"_n;
    static constexpr eosio::name setpropstg         = "setpropstg"_n;
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

struct setpropstg_data {
    name owner;  
    name daocode;                    
    set<name> proposers; 
    set<uint64_t> proposestg ;
};

typedef std::variant<updatedao_data, setpropstg_data> action_data_variant;

class [[eosio::contract("mdao.propose")]] mdaopropose : public contract {

using conf_t = mdao::conf_global_t;
using conf_table_t = mdao::conf_global_singleton;

private:
    dbc                 _db;
    std::unique_ptr<conf_table_t> _conf_tbl_ptr;
    std::unique_ptr<conf_t> _conf_ptr;

    const conf_t& _conf();

public:
    using contract::contract;
    mdaopropose(name receiver, name code, datastream<const char*> ds):_db(_self),  contract(receiver, code, ds){}

    ACTION create(const name& creator,const string& name, const string& desc,
                            const uint64_t& stgid, const uint32_t& votes);

    ACTION cancel(const name& owner, const uint64_t& proposeid);

    ACTION addplan( const name& owner, const uint64_t& proposeid, const string& title );

    ACTION start(const name& owner, const uint64_t& proposeid);

    ACTION excute(const name& owner, const uint64_t& proposeid);

    ACTION votefor(const name& voter, const uint64_t& proposeid, const uint32_t optid);

    ACTION setaction(const name& owner, const uint64_t& proposeid, 
                                const uint32_t& optid,  const name& action_name, 
                                const name& action_account, const std::vector<char>& packed_action_data);
    
    ACTION recycledb(uint32_t max_rows);
    
private:
    void _check_proposal_params(const action_data_variant& data_var,  const name& action_name, const name& action_account);
};