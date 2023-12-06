#pragma once

#include <eosio/asset.hpp>
#include <eosio/eosio.hpp>
#include <eosio/singleton.hpp>
#include <eosio/time.hpp>

#include "mdao.meeting.db.hpp"

using namespace eosio;
using namespace mdao;

class [[eosio::contract("mdao.meeting")]] mdaomeeting : public contract {
private:
    global_t::tbl_t     _global;
    global_t            _gstate;
    meeting_t::tbl_t    _meeting_tbl;

public:
    using contract::contract;

    mdaomeeting(name receiver, name code, datastream<const char*> ds):
        contract(receiver, code, ds), 
        _global(_self, _self.value),
        _meeting_tbl(get_self(), get_self().value)
        {
        _gstate = _global.exists() ? _global.get() : global_t{};
    }

    ~mdaomeeting() {
        _global.set( _gstate, get_self() );    
    }
    
    ACTION init( const name& admin, const asset& fee);

    ACTION setreceiver( const name& receiver);

    ACTION setsplit(const uint64_t& split_id);
    
    [[eosio::on_notify("*::transfer")]]
    void ontransfer(const name& from, const name& to, const asset& quant, const string& memo);
private:
    void _create_renew_dao(const name& from, const name& dao_code, const string& group_id, const asset& quantity, const uint64_t& month);
};