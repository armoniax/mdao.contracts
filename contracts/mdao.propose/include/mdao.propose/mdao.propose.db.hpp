#pragma once

#include <eosio/asset.hpp>
#include <eosio/singleton.hpp>
#include <eosio/privileged.hpp>
#include <eosio/name.hpp>
#include <eosio/action.hpp>
#include <map>

namespace mdao {

using namespace std;
using namespace eosio;

#define TG_TBL [[eosio::table, eosio::contract("mdao.propose")]]

static uint128_t get_union_id(const name& account, const uint64_t& proposal_id){
    return ( (uint128_t)account.value ) << 64 | proposal_id;
}

struct plan{
    uint32_t    id = 0;
    string      title;
    string      desc;
    uint32_t    recv_votes = 0;
};

struct single_plan{
    string      title;
    string      desc;
    uint32_t    recv_votes = 0;
    action      execute_action;
};

struct multiple_plan {
    vector<plan>    plans;
};

typedef std::variant<single_plan, multiple_plan> plan_data;

struct TG_TBL proposal_t {
    uint64_t        id;
    name            dao_code;
    uint64_t        vote_strategy_id;
    name            status;
    name            creator;
    string          proposal_name;
    string          title;
    string          desc;
    name            type;
    uint32_t        recv_votes; //收到总票数
    uint32_t        reject_votes; //弃权总票数
    uint32_t        users_count; //参与人数
    uint32_t        reject_users_count; //拒绝人数
    plan_data       proposal_plan;
    time_point_sec  created_at = current_time_point();
    time_point_sec  started_at;
    time_point_sec  executed_at;

    uint64_t    primary_key()const { return id; }
    uint64_t    scope() const { return 0; }

    proposal_t() {}
    proposal_t(const uint64_t& i): id(i) {}

    EOSLIB_SERIALIZE( proposal_t, (id)(dao_code)(vote_strategy_id)(status)(creator)(proposal_name)(title)(desc)(type)
                (recv_votes)(reject_votes)(users_count)(reject_users_count)(proposal_plan)(created_at)(started_at)(executed_at) )

    typedef eosio::multi_index <"proposals"_n, proposal_t> idx_t;
};

struct TG_TBL votelist_t {
    uint64_t        id;
    name            account;
    name            direction; //agree ｜ reject
    uint64_t        proposal_id;
    uint32_t        plan_id;
    uint32_t        vote_weight;
    time_point_sec  voted_at;

    uint64_t    primary_key()const { return id; }
    uint64_t    scope() const { return 0; }

    votelist_t() {}
    votelist_t(const uint64_t& pid): proposal_id(pid) {}

    uint64_t by_account() const { return account.value; }
    uint128_t by_union_id()const {
        return get_union_id( account, proposal_id);
    }
    EOSLIB_SERIALIZE( votelist_t, (id)(account)(direction)(proposal_id)(plan_id)(vote_weight)(voted_at) )

    typedef eosio::multi_index <"votes"_n, votelist_t,
        indexed_by<"accountid"_n,  const_mem_fun<votelist_t, uint64_t, &votelist_t::by_account> >,
        indexed_by<"unionid"_n,  const_mem_fun<votelist_t, uint128_t, &votelist_t::by_union_id> >
    > idx_t;
};

} //amax