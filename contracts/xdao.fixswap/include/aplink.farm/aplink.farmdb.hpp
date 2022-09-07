#include <thirdparty/wasm_db.hpp>
#include <eosio/eosio.hpp>
#include <eosio/system.hpp>

using namespace std;

namespace aplink {

// static constexpr symbol APLINK_SYMBOL              = SYMBOL("APL", 4);
static constexpr name APLINK_BANK                  { "aplink.token"_n };

using std::string;
using std::vector;
using namespace eosio;


struct [[eosio::table, eosio::contract("aplink.farm")]] land_t {
    uint64_t        id;
    name            farmer;                     //land farmer
    string          title;                      //land title: <=64 chars
    string          uri;                        //land uri: <=64 chars
    string          banner;                        //banner uri: <=64 chars
    asset           avaliable_apples;
    asset           alloted_apples;
    uint8_t         status = 1;         //status of land, see land_status_t
    time_point_sec      opened_at;              //customer can crop at
    time_point_sec      closed_at;                //customer stop crop at
    time_point_sec      created_at;                 //creation time (UTC time)
    time_point_sec      updated_at;                 //update time: last updated atuint8_t  
    
    land_t() {}
    land_t(const uint64_t& pid): id(pid) {}

    uint64_t primary_key() const { return id; }

    uint64_t by_updatedid() const { return ((uint64_t)updated_at.sec_since_epoch() << 32) | (id & 0x00000000FFFFFFFF); }
    uint128_t by_farmer() const { return (uint128_t)farmer.value << 64 | (uint128_t)id; }

    typedef eosio::multi_index<"lands"_n, land_t,
        indexed_by<"updatedid"_n,  const_mem_fun<land_t, uint64_t, &land_t::by_updatedid> >,
        indexed_by<"farmeridx"_n,  const_mem_fun<land_t, uint128_t, &land_t::by_farmer> >
    > idx_t;

    EOSLIB_SERIALIZE( land_t, (id)(farmer)(title)(uri)(banner)(avaliable_apples)(alloted_apples)(status)
    (opened_at)(closed_at)(created_at)(updated_at) )
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
    void allot(const uint64_t& land_id, const name& customer, const asset& quantity, const string& memo);

    using allot_action = eosio::action_wrapper<"allot"_n, &farm::allot>;

    static asset get_avaliable_apples( const name& token_contract_account, const uint64_t& land_id )
    {
        auto db = dbc(token_contract_account);
        auto land = land_t(land_id);
        if (!db.get(land)) return asset(0, APLINK_SYMBOL);
        auto now = time_point_sec(current_time_point());
        if (now < land.opened_at || now > land.closed_at) return asset(0, APLINK_SYMBOL);
        if (land.status != 1) return asset(0, APLINK_SYMBOL);
        
        return land.avaliable_apples;
    }
};
}