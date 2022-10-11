
#pragma once

#include <eosio/asset.hpp>
#include <eosio/singleton.hpp>
#include <eosio/privileged.hpp>
#include <eosio/name.hpp>
#include <eosio/time.hpp>
#include <map>

using namespace eosio;


namespace mdao {

using namespace std;
using namespace eosio;

#define GOV_TG_TBL [[eosio::table, eosio::contract("mdao.gov")]]
// static constexpr uint64_t UNLOCK_TIME = 48 * 3600;
static constexpr uint64_t LOCK_HOURS = 48;
static constexpr uint64_t VOTING_HOURS = 48;

struct GOV_TG_TBL governance_t {
    name                        dao_code;
    map<name, uint64_t>         propose_strategy;
    map<name, uint64_t>         vote_strategy;
    map<uint64_t, uint32_t>     require_participation;// statke -> ratio(100% = 10000), token -> votes
    map<uint64_t, uint32_t>     require_pass;         // statke -> ratio(100% = 10000), token -> votes
    uint16_t                    limit_update_hours      = LOCK_HOURS;
    uint16_t                    voting_limit_hours      = VOTING_HOURS;
    time_point_sec              last_updated_at         = current_time_point();

    uint64_t    primary_key()const { return dao_code.value; }
    uint64_t    scope() const { return 0; }

    governance_t() {}
    governance_t(const name& c): dao_code(c) {}

    EOSLIB_SERIALIZE( governance_t, (dao_code)(propose_strategy)(vote_strategy)(require_participation)(require_pass)(limit_update_hours)(voting_limit_hours)(last_updated_at) )

    typedef eosio::multi_index <"governances"_n, governance_t> idx_t;
};

} //amax