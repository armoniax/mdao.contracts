#pragma once

#include <eosio/asset.hpp>
#include <eosio/eosio.hpp>
#include <string>
#include "hotpotalgoexdb.hpp"


using namespace std;
using namespace eosio;
using namespace wasm::db;


namespace hotpot
{
using std::pair;
using std::string;

class [[eosio::contract("hotpotalgoex")]] algoex : public contract
{
private:
    global_singleton    _global;
    global_t            _gstate;
    dbc           _db;

    void _create_market(const name& creator, const vector<string_view>& memo_params);

    void _allot_tax(const name& account, const market_t& market, const asset& tax, const name& bank_con);

    void _launch_market(const name& launcher, 
                            const asset& quantity,
                            const vector<string_view>& memo_params);

    void _launch_polycurve_market(
                            market_t market,
                            const name& launcher, 
                            const asset& quantity,
                            const asset& lauch_price);

    void _bid(const name& account, const asset& quantity, const symbol_code& base_code);

    void _ask(const name& account, const asset& quantity, const symbol_code& base_code);

public:
    algoex(eosio::name receiver, eosio::name code, datastream<const char *> ds) : _db(_self),contract(receiver, code, ds), _global(_self, _self.value)
    {
        if (_global.exists()) _gstate = _global.get();
        else _gstate = global_t{};
    }

    [[eosio::action]]
    void init(const name &admin);

    [[eosio::action]]
    void setlimitsym(const set<string>& sym_codes);

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
    void updateappinf(const name& creator,
                      const symbol_code& base_code,
                      const AppInfo_t& app_info
                    );

    [[eosio::action]]
    void setlauncher(const name& creator,
                    const symbol_code& base_code, 
                    const name& launcher,
                    const string& recv_memo
                    );

    [[eosio::action]]
    void settaxtaker(const name& creator, 
                    const symbol_code& base_code,
                    const name& taxker,
                    const string& recv_memo
                    );

    [[eosio::action]]
    void setmktstatus(const name& creator, 
                    const symbol_code& base_code,
                    const name& status
                    );
};

}