#include <otcconf/wasm_db.hpp>
#include <eosio/eosio.hpp>
#include <eosio/system.hpp>

using namespace std;

namespace aplink {

static constexpr symbol APLINK_SYMBOL              = SYMBOL("APL", 4);
static constexpr name APLINK_BANK                  { "aplink.token"_n };

using std::string;
using std::vector;
using namespace eosio;

namespace lease_status {
    static constexpr eosio::name none           = "none"_n;
    static constexpr eosio::name active         = "active"_n;
    static constexpr eosio::name inactive       = "inactive"_n;
};

struct [[eosio::table, eosio::contract("aplink.farm")]] lease_t {
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

class [[eosio::contract("aplink.farm")]] farm: public eosio::contract {
public:
    /**
     * @brief farmer can plant seeds to customer
     * 
     * @param land_id 
     * @param customer  send seeds to account
     * @param quantity
     * @param memo 
     */
    [[eosio::action]]
    void allot(const uint64_t& lease_id, const name& farmer, const asset& quantity, const string& memo);

    using allot_action = eosio::action_wrapper<"allot"_n, &farm::allot>;

    static void available_apples( const name& apl_farm_contract, const uint64_t& land_id, asset& apples )
    {
        auto db         = dbc( apl_farm_contract );
        auto lease      = lease_t(land_id);
        auto now        = time_point_sec(current_time_point());

        if (!db.get(lease) ||
            now < lease.opened_at || 
            now > lease.closed_at ||
            lease.status != lease_status::active) 
            apples = asset(0, APLINK_SYMBOL);
        
        apples = lease.available_apples;
    }
};
}