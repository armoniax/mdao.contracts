
#pragma once

#include <eosio/asset.hpp>
#include <eosio/singleton.hpp>
#include <eosio/privileged.hpp>
#include <eosio/name.hpp>
#include <xdao.conf/xdao.conf.db.hpp>

#include "wasm_db.hpp"
#include <map>

using namespace eosio;


namespace xdao {

using namespace std;
using namespace eosio;

namespace bind_status {
    static constexpr eosio::name BIND       = "bound"_n;
    static constexpr eosio::name CONFIRMED  = "confirmed"_n;
};

#define TG_TBL [[eosio::table, eosio::contract("xdao.info")]]

typedef eosio::singleton< "global"_n, global_t > global_singleton;

struct coin_info{
    set<evm_symbol> evmcoins;
    set<ext_symbol> coins;
    uint8_t         coin_seat;
};

struct wallet_info{
    uint64_t wallet_id;
    string   evm_wallet_address;
};

struct dapp_info{
    set<app_info> dapps;
    uint8_t       dapp_seat;
};

struct gover_info{
    uint64_t governance_id;
    string   evm_governance;
};

struct TG_TBL info_t {
    uint64_t            id;
    uint16_t            type;
    uint16_t            status;
    name                creator;
    string              title;
    string              logo;
    string              desc;
    set<string>         tags;
    map<name, string>   links;
    uint16_t            strategy_id;

    coin_info           coin;;
    wallet_info         wallet;
    dapp_info           dapp;
    gover_info          gover;

    uint64_t    primary_key()const { return id; }
    uint64_t    scope() const { return 0; }

    bind_t() {}
    bind_t(const uint64_t& i): id(i) {}
    uint64_t by_creator() const { return creator.value; }

    EOSLIB_SERIALIZE( bind_t, (id)(type)(status)(creator)(title)(logo)(desc)(tags)(links)(strategy_id)(coin)(wallet)(dapp)(gover) )

    typedef eosio::multi_index
    <"binds"_n, info_t,
        indexed_by<"account"_n, const_mem_fun<info_t, uint64_t, &info_t::by_creator> >
    > idx_t;
};

} //amax