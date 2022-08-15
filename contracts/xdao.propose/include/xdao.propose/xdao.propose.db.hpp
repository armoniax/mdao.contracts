#pragma once

#include <eosio/asset.hpp>
#include <eosio/singleton.hpp>
#include <eosio/privileged.hpp>
#include <eosio/name.hpp>
#include <eosio/action.hpp>
#include <map>

namespace xdao {

using namespace std;
using namespace eosio;

#define TG_TBL [[eosio::table, eosio::contract("xdao.propose")]]

static uint128_t get_union_id(const name& account, const uint64_t& proposal_id){
    return ( (uint128_t)account.value ) << 64 | proposal_id;
}

struct option{
    uint32_t    id = 0;
    string      title;
    uint32_t    recv_votes = 0;
    action      excute_action;
};

struct TG_TBL propose_t {
    uint64_t    id;
    name        vote_type;
    name        status;
    name        creator;
    string      name;
    string      desc;
    uint64_t    vote_stgid;
    vector<option> opts;
    uint32_t    req_votes;

    uint64_t    primary_key()const { return id; }
    uint64_t    scope() const { return 0; }

    propose_t() {}
    propose_t(const uint64_t& i): id(i) {}

    EOSLIB_SERIALIZE( propose_t, (id)(vote_type)(status)(creator)(name)(desc)(vote_stgid)(opts)(req_votes) )

    typedef eosio::multi_index <"proposes"_n, propose_t> idx_t;
};

struct TG_TBL vote_t {
    uint64_t    id;
    name        account;
    uint64_t    proposal_id;
    uint32_t    vote_time;

    uint64_t    primary_key()const { return id; }
    uint64_t    scope() const { return 0; }

    vote_t() {}
    vote_t(const uint64_t& proposal): proposal_id(proposal) {}

    uint64_t by_account() const { return account.value; }
    uint128_t by_union_id()const {
        return get_union_id( account, proposal_id);
    }
    EOSLIB_SERIALIZE( vote_t, (id)(account)(proposal_id)(vote_time) )

    typedef eosio::multi_index <"votes"_n, vote_t,
        indexed_by<"accountid"_n,  const_mem_fun<vote_t, uint64_t, &vote_t::by_account> >,
        indexed_by<"unionid"_n,  const_mem_fun<vote_t, uint128_t, &vote_t::by_union_id> >


    > idx_t;
};

} //amax