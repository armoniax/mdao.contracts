#pragma once

#include <eosio/asset.hpp>
#include <eosio/eosio.hpp>
#include <eosio/singleton.hpp>
#include <eosio/time.hpp>

#include "xdao.conf.db.hpp"

using namespace eosio;
using namespace xdao;

class [[eosio::contract("xdao.conf")]] xdaoconf : public contract {
private:
    global_t            _gstate;
    global_singleton    _global;

public:
    using contract::contract;

    xdaoconf(name receiver, name code, datastream<const char*> ds):
        contract(receiver, code, ds), _global(_self, _self.value) {
        if (_global.exists()) {
            _gstate = _global.get();

        } else {
            _gstate = global_t{};
        }
    }

    ~xdaoconf() {
        _global.set( _gstate, get_self() );
    }

    [[eosio::action]]
    void init( const name& feetaker, const app_info& appinfo, const asset& daoupgfee );

    [[eosio::action]]
    void daoconf( const name& feetaker,const app_info& appinfo, const name& status, const asset& daoupgfee );

    [[eosio::action]]
    void seatconf( const uint16_t& amctokenmax, const uint16_t& evmtokenmax, uint16_t& dappmax );

    [[eosio::action]]
    void managerconf( const map<name, name>& managers );

};