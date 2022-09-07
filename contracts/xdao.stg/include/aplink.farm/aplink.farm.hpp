#include "aplink.farmdb.hpp"

using namespace std;
using namespace wasm::db;

namespace aplink {

// static constexpr symbol APLINK_SYMBOL              = SYMBOL("APL", 4);
static constexpr name APLINK_BANK                  { "aplink.token"_n };

enum class err: uint8_t {
   NONE                 = 0,
   TIME_INVALID         = 1,
   RECORD_EXISTING      = 2,
   RECORD_NOT_FOUND     = 3,
   SYMBOL_MISMATCH      = 4,
   PARAM_ERROR          = 5,
   PAUSED               = 6,
   NO_AUTH              = 7,
   NOT_POSITIVE         = 8,
   NOT_STARTED          = 9,
   OVERSIZED            = 10,
   TIME_EXPIRED         = 11,
   NOTIFY_UNRELATED     = 12,
   ACTION_REDUNDANT     = 13,
   ACCOUNT_INVALID      = 14,
   CONTENT_LENGTH_INVALID      = 15,
   NOT_DISABLED          = 16,

};

using std::string;
using std::vector;
using namespace eosio;

class [[eosio::contract("aplink.farm")]] farm: public eosio::contract {
private:
    // global_singleton    _global;
    // global_t            _gstate;
    dbc                 _db;

public:
    using contract::contract;

    farm(eosio::name receiver, eosio::name code, datastream<const char*> ds):
        _db(_self), contract(receiver, code, ds)//,
        // _global(get_self(), get_self().value)
    {
        // _gstate = _global.exists() ? _global.get() : global_t{};
    }

    /**
    * @param lord lord can lend out land
    * @param jamfactory expired apples will send to jamfactory
    */
    [[eosio::action]]
    void setlord(const name& lord, const name& jamfactory);

    /**
     * @brief lease a land to a farmer
     * 
     * @param farmer account who can pick the land, always be a contract
     * @param title the land's name
     * @param uri  the details info of the farmer
     * @param banner  the banner info of the farmer
     * @param opened_at  farmer can pick after open_at
     * @param closed_at farmer can pick before close_at
     */
    [[eosio::action]]
    void lease(const name& farmer, const string& title, const string& uri, const string& banner, const time_point& open_at, const time_point& close_at);

    /**
     * @brief reclaim a land, only for disabled land
     * 
     * @param land_id 
     * @param memo 
     */
    [[eosio::action]]
    void reclaim(const uint64_t& land_id, const string& memo);

    /**
     * @brief 
     * 
     * @param land_id 
     * @param status  0:NONE 1:Enable 2:Disable
     */
    [[eosio::action]]
    void setstatus(const uint64_t& land_id, const uint8_t& status);

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

    /**
     * @brief pick apples
     * 
     * @param picker 
     * @param appleids apple_id array, support lessthan 20 apples
     */
    [[eosio::action]]
    void pick(const name& picker, vector<uint64_t> appleids);

    /**
     * @brief topup seeds for a land
     * @param from 
     * @param to 
     * @param quantity 
     * @param memo  land_id
     */
    [[eosio::on_notify("aplink.token::transfer")]]
    void ontransfer(const name& from, const name& to, const asset& quantity, const string& memo);

    using allot_action = eosio::action_wrapper<"allot"_n, &farm::allot>;

        /**
     * @brief 
     * 
     * @param apl_farm_contract 
     * @param land_id 
     * @param apples - available apples
     */
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