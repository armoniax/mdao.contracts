#pragma once

#include <eosio/asset.hpp>
#include <eosio/eosio.hpp>
#include <string>
#include <algorithm>
#include "c2swapdb.hpp"

using namespace std;
using namespace eosio;
using namespace wasm::db;

namespace xdao
{
using std::pair;
using std::string;

class [[eosio::contract("c2swap")]] c2swap : public contract
{
    using contract::contract;

private:
    gswap_singleton    _global;
    gswap_t            _gstate;
    dbc           _db;

public:
    c2swap(eosio::name receiver, eosio::name code, datastream<const char *> ds) : _db(_self),contract(receiver, code, ds), _global(_self, _self.value)
    {
        if (_global.exists()) _gstate = _global.get();
        else _gstate = gswap_t{};
    }

    [[eosio::action]]
    void init(const name& fee_collector, const uint32_t& fee_ratio);

    [[eosio::action]]
    void setfee(const name& fee_collector, const uint32_t& fee_ratio);

    [[eosio::action]]
    void setfarm(const uint64_t& farm_lease_id, const map<extended_symbol, uint32_t>& farm_scales);

    /**
     * action trigger by transfer()
     * @param from from account name
     * @param to must be this contract name
     * @param quantity transfer quantity
     * @param memo memo
     *          make:{taker(optional)}:{take_quantity}:{take_contract}:{code(optional)}
     *              taker: account who can swap quantity, for everybody if empty
     *              take_quantity: asset to swap from taker
     *              take_contract: the asset contract name
     *              code: the hashcode for swap code, for everybody if empty
     *          take:{swap_id}:{code(optional)}
     *              swap_id: swap order id
     *              code: the plain text of swap code
     */
    [[eosio::on_notify("*::transfer")]] 
    void ontransfer(name from, name to, asset quantity, string memo);

    /**
     * cancel, cancel a swap order and getback asset
     */
    [[eosio::action]]
    void cancel(const name& maker, const uint64_t& swap_id);
};

}
