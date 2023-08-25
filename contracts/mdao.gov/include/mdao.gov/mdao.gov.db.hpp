
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

namespace propose_model_type {
    static constexpr name MIX        = "mix"_n;
    static constexpr name PROPOSAL   = "proposal"_n;
};

struct GOV_TG_TBL governance_t {
    name                        dao_code;
    map<name, uint64_t>         strategies;
    uint32_t                    require_participation;// stake -> ratio(100% = 10000), token -> votes
    uint32_t                    require_pass;         // stake -> ratio(100% = 10000), token -> votes
    uint16_t                    update_interval    = LOCK_HOURS;
    uint16_t                    voting_period      = VOTING_HOURS;
    time_point_sec              updated_at;
    name                        proposal_model     = propose_model_type::MIX;
    uint64_t    primary_key()const { return dao_code.value; }
    uint64_t    scope() const { return 0; }

    governance_t() {}
    governance_t(const name& c): dao_code(c) {}

    EOSLIB_SERIALIZE( governance_t, (dao_code)(strategies)(require_participation)(require_pass)(update_interval)(voting_period)(updated_at)(proposal_model) )

    typedef eosio::multi_index <"governances"_n, governance_t> idx_t;
};

} //amax