#pragma once

#include <eosio/asset.hpp>
#include <eosio/singleton.hpp>
#include <eosio/privileged.hpp>
#include <eosio/name.hpp>
#include <eosio/transaction.hpp>
#include <map>
#include <amax.ntoken/amax.ntoken.hpp>

namespace mdao {

using namespace std;
using namespace eosio;
using namespace amax;

#define TG_TBL [[eosio::table, eosio::contract("mdao.propose")]]
#define PROPOSE_TABLE_NAME(name) [[eosio::table(name), eosio::contract("mdao.propose")]]
typedef std::variant<asset, nasset> refasset;

static uint128_t get_union_id(const name& account, const uint64_t& proposal_id){
    return ( (uint128_t)account.value ) << 64 | proposal_id;
}

struct withdraw_str{
    string      option_key;
    uint64_t    vote_id;
};

struct option{
    string      title;
    uint32_t    recv_votes = 0;
};

struct TG_TBL proposal_t {
    uint64_t        id;
    name            dao_code;
    uint64_t        vote_strategy_id;
    uint64_t        proposal_strategy_id;
    name            status;
    name            creator;
    string          title;
    string          desc;
    name            type;
    time_point_sec  created_at = current_time_point();
    time_point_sec  executed_at;
    map<string, option>    options;

    uint64_t    primary_key()const { return id; }
    uint64_t    scope() const { return 0; }
    uint64_t    by_creator()const {
        return creator.value;
    }
    uint64_t    by_daocode()const {
        return dao_code.value;
    }
    uint128_t by_union_id()const {
        return get_union_id( creator, dao_code.value );
    }
    proposal_t() {}
    proposal_t(const uint64_t& i): id(i) {}

    EOSLIB_SERIALIZE( proposal_t, (id)(dao_code)(vote_strategy_id)(proposal_strategy_id)(status)(creator)(title)(desc)(type)
                                    (created_at)(executed_at)(options) )

    typedef eosio::multi_index <"proposals"_n, proposal_t,
        indexed_by<"creator"_n,  const_mem_fun<proposal_t, uint64_t, &proposal_t::by_creator> >,
        indexed_by<"daocode"_n,  const_mem_fun<proposal_t, uint64_t, &proposal_t::by_daocode> >,
        indexed_by<"unionid"_n,  const_mem_fun<proposal_t, uint128_t, &proposal_t::by_union_id> >
    > idx_t;
};

struct TG_TBL vote_t {
    uint64_t        id;
    name            account;
    uint64_t        proposal_id;
    string          option_key;
    uint32_t        vote_weight;
    refasset        quantity;
    name            stg_type;
    time_point_sec  voted_at;

    uint64_t    primary_key()const { return id; }
    uint64_t    scope() const { return 0; }

    vote_t() {}
    vote_t(const uint64_t& pid): proposal_id(pid) {}

    uint64_t by_account() const { return account.value; }
    uint128_t by_union_id()const {
        return get_union_id( account, proposal_id);
    }
    EOSLIB_SERIALIZE( vote_t, (id)(account)(proposal_id)(option_key)(vote_weight)(quantity)(stg_type)(voted_at) )

    typedef eosio::multi_index <"votes"_n, vote_t,
        indexed_by<"accountid"_n,  const_mem_fun<vote_t, uint64_t, &vote_t::by_account> >,
        indexed_by<"unionid"_n,  const_mem_fun<vote_t, uint128_t, &vote_t::by_union_id> >
    > idx_t;
};

struct PROPOSE_TABLE_NAME("global") prop_global_t {
    uint64_t last_propose_id = 0;
    uint64_t last_vote_id = 0;
    EOSLIB_SERIALIZE( prop_global_t, (last_propose_id)(last_vote_id) )
};

typedef eosio::singleton< "global"_n, prop_global_t > propose_global_singleton;
} //amax