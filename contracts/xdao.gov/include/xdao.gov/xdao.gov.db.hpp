
#pragma once

#include <eosio/asset.hpp>
#include <eosio/singleton.hpp>
#include <eosio/privileged.hpp>
#include <eosio/name.hpp>
#include <eosio/time.hpp>
#include <map>

using namespace eosio;


namespace xdao {

using namespace std;
using namespace eosio;

#define GOV_TG_TBL [[eosio::table, eosio::contract("xdao.gov")]]

struct GOV_TG_TBL gov_t {
    name            dao_name;
    name            status;
    name            creator;
    string          title;
    string          desc;
    set<name>       proposers;
    set<uint64_t>   propose_strategies;
    set<uint64_t>   vote_strategies;
    time_point_sec  created_at;
    uint32_t        min_votes;

    uint64_t    primary_key()const { return dao_name.value; }
    uint64_t    scope() const { return 0; }

    gov_t() {}
    gov_t(const name& c): dao_name(c) {}

    EOSLIB_SERIALIZE( gov_t, (dao_name)(status)(creator)(title)(desc)(proposers)(propose_strategies)(vote_strategies)(created_at)(min_votes) )

    typedef eosio::multi_index <"govs"_n, gov_t> idx_t;
};

} //amax