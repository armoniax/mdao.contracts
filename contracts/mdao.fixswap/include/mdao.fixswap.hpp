#pragma once

#include <eosio/asset.hpp>
#include <eosio/eosio.hpp>
#include <string>
#include <algorithm>
#include "mdao.fixswapdb.hpp"
#include <thirdparty/utils.hpp>
#include <eosio.token/eosio.token.hpp>
using namespace std;
using namespace eosio;
using namespace wasm::db;

namespace mdao
{
using std::pair;
using std::string;
static constexpr name      NFT_BANK    = "amax.ntoken"_n;

class [[eosio::contract("fixswap")]] fixswap : public contract
{
    using contract::contract;

private:
    gswap_singleton    _global;
    gswap_t            _gstate;
    dbc                _db;

    void _transaction_transfer(const extended_asset& make_asset, 
                                    const extended_asset& take_asset, 
                                    const name& maker, 
                                    const name& taker, 
                                    const name& order_no);

    void _reward_transfer(const extended_asset& make_asset, 
                                    const extended_asset& take_asset, 
                                    const name& maker, 
                                    const name& taker, 
                                    const name& order_no);

public:
    fixswap(eosio::name receiver, eosio::name code, datastream<const char *> ds) : _db(_self),contract(receiver, code, ds), _global(_self, _self.value)
    {
        if (_global.exists()) _gstate = _global.get();
        else _gstate = gswap_t{};
    }

    [[eosio::action]]
    void init(const name& admin, const name& fee_collector, const uint32_t& fee_ratio);

    [[eosio::action]]
    void setfee(const name& fee_collector, const uint32_t& take_fee_ratio, const uint32_t& make_fee_ratio);

    [[eosio::action]]
    void setfarm(const uint64_t& farm_lease_id, const map<extended_symbol, int64_t>& farm_scales);

    /**
     * action trigger by transfer()
     * @param from from account name
     * @param to must be this contract name
     * @param quantity transfer quantity
     * @param memo memo
     *          make:{orderno}:{taker(optional)}:{take_quantity}:{take_contract}:{code(optional)}
     *              orderno: orderno for swap
     *              taker: account who can swap quantity, for everybody if empty
     *              take_quantity: asset to swap from taker
     *              take_contract: the asset contract name
     *              code: the hashcode for swap code, for everybody if empty
     *          take:{orderno}:{code(optional)}
     *              orderno: swap orderno
     *              code: the plain text of swap code
     */
    [[eosio::on_notify("*::transfer")]]
    void ontransfer(name from, name to, asset quantity, string memo);

    // [[eosio::on_notify("amax.ntoken::transfer")]] 
    // void onnftsawp(name from, name to, vector< nasset >& assets, string memo);

    /**
     * cancel, cancel a swap order and getback asset
     */
    [[eosio::action]]
    void cancel(const name& maker, const name& orderno);

    // [[eosio::action]]
    // void clear(const vector<name>& order_nos);

    [[eosio::action]]
    void notification(const name& order_no, const name& status);

    using notification_action = eosio::action_wrapper<"notification"_n, &fixswap::notification>;

};

}
