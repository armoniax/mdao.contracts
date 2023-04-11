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
#define GROUPTHR_TABLE_NAME(name) [[eosio::table(name), eosio::contract("mdaogroupthr")]]

typedef std::variant<extended_nasset, extended_asset> refasset;

struct deleted_member {
    uint64_t           groupthr_id;
    name               account;
    EOSLIB_SERIALIZE(deleted_member, (groupthr_id)(account) )
};

struct GROUPTHR_TG_TBL groupthr_t {
    uint64_t            id;
    name                owner;
    string              group_id;
    name                threshold_type;
    map<name, refasset> threshold_plan;
    bool                enable_threshold = true;
    time_point_sec      expired_time;
    uint64_t            primary_key()const { return id; }
    uint64_t            scope() const { return 0; }
    checksum256         by_group_id() const { return HASH256(group_id); }

    groupthr_t() {}
    groupthr_t(const uint64_t& id): id(id) {}

    EOSLIB_SERIALIZE( groupthr_t, (id)(owner)(group_id)(threshold_type)(threshold_plan)(enable_threshold)(expired_time))

    typedef eosio::multi_index< "groupthr"_n, groupthr_t,
            indexed_by<"bygroupid"_n, const_mem_fun<groupthr_t, checksum256, &groupthr_t::by_group_id>>
    > idx_t;
};

struct GROUPTHR_TG_TBL member_t {
    uint64_t            id;
    uint64_t            groupthr_id;
    time_point_sec      expired_time;
    name                member;
    name                type;

    uint64_t            primary_key()const { return id; }
    uint64_t            scope() const { return 0; }
    uint128_t           by_id_groupthrid() const { return (uint128_t)member.value << 64 | (uint128_t)groupthr_id; }

    member_t() {}
    member_t(const uint64_t& id): id(id) {}

    EOSLIB_SERIALIZE( member_t, (id)(groupthr_id)(expired_time)(member)(type))

    typedef eosio::multi_index< "members"_n, member_t,
            indexed_by<"byidgroupid"_n, const_mem_fun<member_t, uint128_t, &member_t::by_id_groupthrid>>
    > idx_t;

};

struct GROUPTHR_TABLE_NAME("global") thr_global_t {
    uint64_t last_groupthr_id = 0;
    uint64_t last_member_id   = 0;
    asset crt_groupthr_fee    = asset(100000000,AMAX_SYMBOL);
    asset join_member_fee     = asset(0,AMAX_SYMBOL);

    EOSLIB_SERIALIZE( thr_global_t, (last_groupthr_id)(last_member_id)(crt_groupthr_fee)(join_member_fee) )
};

typedef eosio::singleton< "global"_n, thr_global_t > groupthr_global_singleton;

} //mdao