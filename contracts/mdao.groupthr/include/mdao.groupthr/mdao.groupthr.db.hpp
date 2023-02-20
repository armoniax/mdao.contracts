#pragma once

#include <eosio/asset.hpp>
#include <eosio/singleton.hpp>
#include <eosio/privileged.hpp>
#include <eosio/name.hpp>
#include <eosio/action.hpp>
#include <amax.ntoken/amax.ntoken.hpp>
#include <map>

namespace mdao {

using namespace std;
using namespace amax;
using namespace eosio;

#define GROUPTHR_TG_TBL [[eosio::table, eosio::contract("mdaogroupthr")]]

typedef std::variant<extended_nasset, extended_asset> refasset;

struct GROUPTHR_TG_TBL groupthr_t {
    uint64_t            id;
    name                dao_code;
    string              group_id;
    name                type;
    refasset            threshold;
    time_point_sec      expired_time;
    uint64_t    primary_key()const { return id; }
    uint64_t    scope() const { return 0; }
    checksum256 by_group_id() const { return HASH256(group_id); }

    groupthr_t() {}
    groupthr_t(const uint64_t& id): id(id) {}

    EOSLIB_SERIALIZE( groupthr_t, (id)(dao_code)(group_id)(type)(threshold)(expired_time))

    typedef eosio::multi_index< "groupthr"_n, groupthr_t,
            indexed_by<"bygroupid"_n, const_mem_fun<groupthr_t, checksum256, &groupthr_t::by_group_id>>
    > idx_t;
};

struct GROUPTHR_TG_TBL mermber_t {
    uint64_t            id;
    uint64_t            groupthr_id;
    time_point_sec      expired_time;
    name                mermber;
    uint64_t    primary_key()const { return id; }
    uint64_t    scope() const { return 0; }
    uint128_t   by_id_groupthrid() const { return (uint128_t)mermber.value << 64 | (uint128_t)groupthr_id; }

    mermber_t() {}
    mermber_t(const uint64_t& id): id(id) {}

    EOSLIB_SERIALIZE( mermber_t, (id)(groupthr_id)(expired_time)(mermber))

    typedef eosio::multi_index< "mermbers"_n, mermber_t,
        indexed_by<"byidgroupid"_n, const_mem_fun<mermber_t, uint128_t, &mermber_t::by_id_groupthrid>>
    > idx_t;

};
} //amax