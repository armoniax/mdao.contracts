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
    INSUFFICIENT_BALANCE    = 25,
    VOTING                  = 26,
    STRATEGY_STATUS_ERROR   = 27,
    INVALID_FORMAT          = 28,
    NOT_VOTED               = 29,
    NO_SUPPORT              = 30
};

class [[eosio::contract("mdao.propose")]] mdaoproposal : public contract {

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
    ACTION init(const uint64_t& last_propose_id, const uint64_t& last_vote_id);

    ACTION removeglobal();
    
    ACTION create(const name& creator, const name& dao_code, const string& title, const string& desc, map<string, string> options);

    ACTION cancel(const name& owner, const uint64_t& proposal_id);

    ACTION votefor(const name& voter, const uint64_t& proposal_id,  const string& title);
              
    ACTION withdraw(const vector<withdraw_str>& withdraws);
    // ACTION recycledb(uint32_t max_rows);

    // ACTION deletepropose(uint64_t id);
    // ACTION deletevote(uint32_t id); 

private:
    void _cal_votes(const name dao_code, const strategy_t& vote_strategy, const name voter, weight_struct& weight_str, const uint32_t& lock_time, const int128_t& voting_rate) ;
};
