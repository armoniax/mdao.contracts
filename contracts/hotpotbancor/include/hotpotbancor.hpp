#pragma once

#include <eosio/asset.hpp>
#include <eosio/eosio.hpp>
#include <string>
#include "hotpotbancordb.hpp"


using namespace std;
using namespace eosio;
using namespace wasm::db;


namespace hotpot
{
using std::pair;
using std::string;

class [[eosio::contract("hotpotbancor")]] bancor : public contract
{
private:
    global_singleton    _global;
    global_t            _gstate;
    dbc           _db;

    asset _convert_to_exchange(market_t& market, extended_asset& reserve, const asset& payment );
    asset _convert_from_exchange(market_t& market, extended_asset& reserve, const asset& tokens );
    asset _convert(market_t& market, const asset& from, const symbol& to );
    
    void _create_market(const name& creator, 
                        const uint16_t& cw_value,
                        const extended_asset& base_supply,
                        const extended_asset& quote_supply,
                        const uint16_t& in_tax,
                        const uint16_t& out_tax,
                        const uint16_t& parent_rwd_rate,
                        const uint16_t& grand_rwd_rate
                        );

    void _allot_tax(const name& account, const market_t& market, const asset& tax, const name& bank_con);


    void _launch_market(const name& creator, 
                            const asset& quantity,
                            const symbol_code& base_code);

    void _bid(const name& account, const asset& quantity, const symbol_code& base_code);

    void _ask(const name& account, const asset& quantity, const symbol_code& base_code);

public:
    bancor(eosio::name receiver, eosio::name code, datastream<const char *> ds) : _db(_self),contract(receiver, code, ds), _global(_self, _self.value)
    {
        if (_global.exists()) _gstate = _global.get();
        else _gstate = global_t{};
    }

    [[eosio::action]]
    void init(const name &admin);

    [[eosio::action]]
    void setadmin(const name& admin_type,const name &admin);

    [[eosio::action]]
    void setstatus(const name& status_type);

    /**
     * ontransfer, trigger by recipient of transfer()
     * memo:
     *      creat:{}
     */
    [[eosio::on_notify("*::transfer")]] 
    void ontransfer(name from, name to, asset quantity, string memo);

    /**
     * ontransfer, trigger by recipient of issue()
     */
    [[eosio::on_notify("hotpotxtoken::issue")]] 
    void onissue(const name &to, const asset &quantity, const string &memo);

    [[eosio::action]]
    void updateappinf(const uint64_t& market_id,
                      const AppInfo_t& app_info
                    );
};

}
