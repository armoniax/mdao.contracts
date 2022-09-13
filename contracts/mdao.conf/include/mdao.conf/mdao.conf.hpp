#pragma once

#include <eosio/asset.hpp>
#include <eosio/eosio.hpp>
#include <eosio/singleton.hpp>
#include <eosio/time.hpp>

#include "mdao.conf.db.hpp"

using namespace eosio;
using namespace mdao;

class [[eosio::contract("mdaoconftest")]] mdaoconf : public contract {
private:
    conf_global_t            _gstate;
    conf_global_singleton    _global;

public:
    using contract::contract;

    mdaoconf(name receiver, name code, datastream<const char*> ds):
        contract(receiver, code, ds), _global(_self, _self.value) {
        if (_global.exists()) {
            _gstate = _global.get();

        } else {
            _gstate = conf_global_t{};
        }
    }

    ~mdaoconf() {
        _global.set( _gstate, get_self() );

    }

    [[eosio::action]]
    void init( const name& feetaker, const app_info& appinfo, const asset& daoupgfee, const name& admin );

    [[eosio::action]]
    void daoconf( const name& feetaker,const app_info& appinfo, const name& status, const asset& daoupgfee );

    [[eosio::action]]
    void seatconf( const uint16_t& amctokenmax, const uint16_t& evmtokenmax, uint16_t& dappmax );

    [[eosio::action]]
    ACTION managerconf( const name& managetype, const name& manager );

    [[eosio::action]]
    void setlimitcode( const symbol_code& symbolcode );

    [[eosio::action]]
    void reset();
};