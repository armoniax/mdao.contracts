#pragma once

#include "wasm_db.hpp"

#include <eosio/eosio.hpp>
#include <eosio/asset.hpp>
#include <eosio/privileged.hpp>
#include <eosio/singleton.hpp>
#include <eosio/system.hpp>
#include <eosio/time.hpp>

using namespace eosio;
using namespace std;
using std::string;

// using namespace wasm;
#define SYMBOL(sym_code, precision) symbol(symbol_code(sym_code), precision)

static constexpr eosio::name active_perm        {"active"_n};
static constexpr symbol SYS_SYMBOL              = SYMBOL("AMAX", 8);
static constexpr name SYS_BANK                  { "amax.token"_n };

#ifndef DAY_SECONDS_FOR_TEST
static constexpr uint64_t DAY_SECONDS           = 24 * 60 * 60;
#else
#warning "DAY_SECONDS_FOR_TEST should be used only for test!!!"
static constexpr uint64_t DAY_SECONDS           = DAY_SECONDS_FOR_TEST;
#endif//DAY_SECONDS_FOR_TEST

static constexpr uint32_t MAX_TITLE_SIZE        = 64;

namespace wasm { namespace db {

#define TG_TBL [[eosio::table, eosio::contract("xdaoredpackx")]]
#define TG_TBL_NAME(name) [[eosio::table(name), eosio::contract("xdaoredpackx")]]

struct TG_TBL_NAME("global") global_t {
    name tg_admin;
    uint16_t expire_hours;
    uint16_t data_failure_hours;
    EOSLIB_SERIALIZE( global_t, (tg_admin)(expire_hours)(data_failure_hours) )
};
typedef eosio::singleton< "global"_n, global_t > global_singleton;


namespace redpack_status {
    static constexpr eosio::name CREATED     = "created"_n;
    static constexpr eosio::name FINISHED    = "finished"_n;
    static constexpr eosio::name CANCELLED    = "cancelled"_n;

};

uint128_t get_unionid( const name& rec, uint64_t packid ) {
     return ( (uint128_t) rec.value << 64 ) | (packid & 0x00000000FFFFFFFF);
}

struct TG_TBL redpack_t {
    uint64_t        id;
    name            sender;
    string          pw_hash;
    asset           total_quantity;
    uint64_t        receiver_count;
    asset           remain_quantity;
    uint64_t        remain_count         = 0;
    asset           fee;
    name            status;
    uint16_t        type;  //0 random,1 mean
    time_point      created_at;
    time_point      updated_at;

    uint64_t primary_key() const { return id; }

    uint64_t by_updatedid() const { return ((uint64_t)updated_at.sec_since_epoch() << 32) | (id & 0x00000000FFFFFFFF); }
    uint64_t by_sender() const { return sender.value; }

    redpack_t(){}
    redpack_t( const uint64_t& pkid ): id(pkid){}

    typedef eosio::multi_index<"redpacks"_n, redpack_t,
        indexed_by<"updatedid"_n,  const_mem_fun<redpack_t, uint64_t, &redpack_t::by_updatedid> >,
        indexed_by<"senderid"_n,  const_mem_fun<redpack_t, uint64_t, &redpack_t::by_sender> >
    > idx_t;

    EOSLIB_SERIALIZE( redpack_t, (id)(sender)(pw_hash)(total_quantity)(receiver_count)(remain_quantity)
                              (remain_count)(fee)(status)(type)(created_at)(updated_at) )
};

struct TG_TBL claim_t {
    uint64_t        id;
    uint64_t        pack_id;
    name            sender;                     //plan owner
    name            receiver;                      //plan title: <=64 chars
    asset           quantity;             //asset issuing contract (ARC20)
    time_point      claimed_at;                 //update time: last updated at
    uint64_t primary_key() const { return id; }
    uint128_t by_unionid() const { return get_unionid(receiver, pack_id); }
    uint64_t by_claimedid() const { return ((uint64_t)claimed_at.sec_since_epoch() << 32) | (id & 0x00000000FFFFFFFF); }
    uint64_t by_sender() const { return sender.value; }
    uint64_t by_receiver() const { return receiver.value; }
    uint64_t by_packid() const { return pack_id; }

    typedef eosio::multi_index<"claims"_n, claim_t,
        indexed_by<"unionid"_n,  const_mem_fun<claim_t, uint128_t, &claim_t::by_unionid> >,
        indexed_by<"claimedid"_n,  const_mem_fun<claim_t, uint64_t, &claim_t::by_claimedid> >,
        indexed_by<"packid"_n,  const_mem_fun<claim_t, uint64_t, &claim_t::by_packid> >,
        indexed_by<"senderid"_n,  const_mem_fun<claim_t, uint64_t, &claim_t::by_sender> >,
        indexed_by<"receiverid"_n,  const_mem_fun<claim_t, uint64_t, &claim_t::by_receiver> >
    > idx_t;

    EOSLIB_SERIALIZE( claim_t, (id)(pack_id)(sender)(receiver)(quantity)(claimed_at) )
};

struct TG_TBL fee_t {
    symbol          coin;         //co-PK
    asset           fee;
    name            contract_name;
    uint16_t        min_unit;

    fee_t() {};
    fee_t( const symbol& co ): coin( co ) {}

    uint64_t primary_key()const { return coin.code().raw(); }

    typedef eosio::multi_index< "fees"_n,  fee_t > idx_t;

    EOSLIB_SERIALIZE( fee_t, (coin)(fee)(contract_name)(min_unit) );
};


} }