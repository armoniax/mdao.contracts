#pragma once

#include <eosio/asset.hpp>
#include <eosio/eosio.hpp>
#include <eosio/singleton.hpp>
#include <eosio/time.hpp>

#include "mdao.conf.db.hpp"

using namespace eosio;
using namespace mdao;

class [[eosio::contract("mdao.conf")]] mdaoconf : public contract {
private:
    conf_global_t            _gstate;
    conf_global_singleton    _global;
    conf_global_t2           _gstate2;
    conf_global_singleton2   _global2;
    conf_global_t3           _gstate3;
    conf_global_singleton3   _global3;

public:
    using contract::contract;

    mdaoconf(name receiver, name code, datastream<const char*> ds):
        contract(receiver, code, ds), _global(_self, _self.value), _global2(_self, _self.value), _global3(_self, _self.value) {
        if (_global.exists()) {
            _gstate = _global.get();

        } else {
            _gstate = conf_global_t{};
        }

        if (_global2.exists()) {
            _gstate2 = _global2.get();

        } else {
            _gstate2 = conf_global_t2{};
        }
        
        if (_global3.exists()) {
            _gstate3 = _global3.get();

        } else {
            _gstate3 = conf_global_t3{};
        }
    }

    ~mdaoconf() {
        _global.set( _gstate, get_self() );
        _global2.set( _gstate2, get_self() );
        _global3.set( _gstate3, get_self() );
        // _global.remove();

    }

    ACTION migrate() {
        _gstate.status          = conf_status::RUNNING;
    }

    ACTION init( const name& fee_taker, const app_info& app_info, const asset& dao_upg_fee, const name& admin, const name& status );
    ACTION setseat( uint16_t& dappmax );
    ACTION setmeeting( bool& meeting_switch );
    ACTION setmanager( const name& manage_type, const name& manager );
    ACTION setsystem( const name& token_contract, const name& ntoken_contract, uint16_t stake_delay_days );
    ACTION setmetaverse( const bool& enable_metaverse );
    ACTION settokenfee( const asset& quantity );
    ACTION settag( const string& tag );
    ACTION deltag( const string& tag );
    ACTION settokencrtr( const name& creator );
    ACTION deltokencrtr( const name& creator );
    ACTION setplanid( const uint64_t& planid );
    ACTION setthreshold( const asset& threshold );
    ACTION setblacksym( const symbol_code& sym );

};