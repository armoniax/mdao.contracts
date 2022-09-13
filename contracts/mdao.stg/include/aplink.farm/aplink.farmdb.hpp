#pragma once

#include <thirdparty/wasm_db.hpp>
#include <eosio/eosio.hpp>
#include <eosio/asset.hpp>
#include <eosio/privileged.hpp>
#include <eosio/singleton.hpp>
#include <eosio/system.hpp>
#include <eosio/time.hpp>

using namespace eosio;
using namespace std;
using std::string;

using namespace wasm;
#define SYMBOL(sym_code, precision) symbol(symbol_code(sym_code), precision)

// static constexpr name APLINK_BANK                  { "aplink.token"_n };
// static constexpr symbol APLINK_SYMBOL              = SYMBOL("APL", 4);
static constexpr uint32_t MAX_CONTENT_SIZE         = 64;

namespace wasm { namespace db {

#define FARM_TBL [[eosio::table, eosio::contract("aplink.farm")]]
#define FARM_TBL_NAME(name) [[eosio::table(name), eosio::contract("aplink.farm")]]

// struct FARM_TBL_NAME("global") global_t {
//     name landlord;
//     name jamfactory;
//     uint64_t last_lease_id;
//     uint64_t last_allot_id;
    
//     EOSLIB_SERIALIZE( global_t, (landlord)(jamfactory)(last_lease_id)(last_allot_id) )

    // template<typename DataStream>
    // friend DataStream& operator << ( DataStream& ds, const global_t& t ) {
    //     return ds   << t.landlord
    //                 << t.jamfactory
    //                 << t.last_lease_id
    //                 << t.last_allot_id;
    // }

    // template<typename DataStream>
    // friend DataStream& operator >> ( DataStream& ds, global_t& t ) {  
    //     return ds;
    // } 
// };
// typedef eosio::singleton< "global"_n, global_t > global_singleton;


namespace lease_status {
    static constexpr eosio::name none           = "none"_n;
    static constexpr eosio::name active         = "active"_n;
    static constexpr eosio::name inactive       = "inactive"_n;
};
struct FARM_TBL lease_t {
    uint64_t            id;
    name                tenant;                     //tenant (contract account)
    string              land_title;                 //land title: <=64 chars
    string              land_uri;                   //land uri: <=64 chars
    string              banner_uri;                 //banner uri: <=64 chars
    asset               alloted_apples =  asset(0, APLINK_SYMBOL);
    asset               available_apples = asset(0, APLINK_SYMBOL); //total = alloted + available
    name                status = lease_status::active;
    time_point_sec      opened_at;                  //customer can crop at
    time_point_sec      closed_at;                  //customer stop crop at
    time_point_sec      created_at;                 //creation time (UTC time)
    time_point_sec      updated_at;                 //update time: last updated atuint8_t  
    
    lease_t() {}
    lease_t(const uint64_t& pid): id(pid) {}

    uint64_t primary_key() const { return id; }

    uint128_t by_tenant() const { return (uint128_t)tenant.value << 64 | (uint128_t)id; }
    uint64_t by_updatedid() const { return ((uint64_t)updated_at.sec_since_epoch() << 32) | (id & 0x00000000FFFFFFFF); }
    typedef eosio::multi_index<"leases"_n, lease_t,
        indexed_by<"tenantidx"_n,  const_mem_fun<lease_t, uint128_t, &lease_t::by_tenant> >,
        indexed_by<"updatedid"_n,  const_mem_fun<lease_t, uint64_t, &lease_t::by_updatedid> >
    > idx_t;

    EOSLIB_SERIALIZE( lease_t,  (id)(tenant)(land_title)(land_uri)(banner_uri)(alloted_apples)(available_apples)
                                (status)(opened_at)(closed_at)(created_at)(updated_at) )

};

struct FARM_TBL allot_t {
    uint64_t        id;
    uint64_t        lease_id;                   //land lease id
    name            farmer;                     //land farmer who picks apples
    asset           apples;                     //can only be picked or reclaimed all at once
    time_point_sec  alloted_at;
    time_point_sec  expired_at;                 //expire time (UTC time)

    allot_t() {}
    allot_t(const uint64_t& pid): id(pid) {}
    uint64_t primary_key() const { return id; }

    uint128_t by_farmer() const { return (uint128_t)farmer.value << 64 | (uint128_t)id; }
    uint64_t by_expiry() const { return ((uint64_t)expired_at.sec_since_epoch() << 32) | (id & 0x00000000FFFFFFFF); }

    typedef eosio::multi_index<"allots"_n, allot_t,
        indexed_by<"farmeridx"_n,  const_mem_fun<allot_t, uint128_t, &allot_t::by_farmer> >,
        indexed_by<"expireidx"_n,  const_mem_fun<allot_t, uint64_t, &allot_t::by_expiry> >
    > idx_t;

    EOSLIB_SERIALIZE( allot_t, (id)(lease_id)(farmer)(apples)(alloted_at)(expired_at) )
};

} }