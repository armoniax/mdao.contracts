
#pragma once

#include <eosio/asset.hpp>
#include <eosio/singleton.hpp>
#include <eosio/privileged.hpp>
#include <eosio/name.hpp>
#include <xdao.info/xdao.info.db.hpp>

#include "wasm_db.hpp"
#include <map>

using namespace eosio;


namespace xdao {

#define TG_TBL [[eosio::table, eosio::contract("xdao.info")]]

struct evm_symbol{
    string address;
    string symbol;
    friend bool operator < (const evm_symbol& symbol1, const evm_symbol& symbol2);
    EOSLIB_SERIALIZE(evm_symbol, (address)(symbol) )
};

bool operator < (const evm_symbol& symbol1, const evm_symbol& symbol2) {
    return symbol1.address < symbol2.address;
}


struct TG_TBL evm_info_t {
    name            code;
    string          evm_wallet_address;
    string          evm_governance;
    set<evm_symbol> evmtokens;
    string          chain;

    uint64_t    primary_key()const { return code.value; }
    uint64_t    scope() const { return 0; }

    evm_info_t() {}
    evm_info_t(const name& c): code(c) {}

    typedef eosio::multi_index<"evminfos"_n, evm_info_t> idx_t;

    EOSLIB_SERIALIZE( evm_info_t, (code)(evm_wallet_address)(evm_governance)(evmtokens)(chain) )

};

struct TG_TBL amc_info_t {
    name                 code;
    optional<uint64_t>   wallet_id;
    optional<uint64_t>   governance_id;
    set<extended_symbol> tokens;

    uint64_t    primary_key()const { return code.value; }
    uint64_t    scope() const { return 0; }

    amc_info_t() {}
    amc_info_t(const name& c): code(c) {}

    typedef eosio::multi_index<"amcinfos"_n, amc_info_t> idx_t;

    EOSLIB_SERIALIZE( amc_info_t, (code)(wallet_id)(governance_id)(tokens))

};

struct TG_TBL details_t {
    name                code;
    name                type;
    name                status;
    name                creator;
    string              title;
    string              logo;
    string              desc;
    set<string>         tags;
    map<name, string>   links;
    optional<uint64_t>   strategy_id;
    set<app_info>       dapps;

    details_t() {}
    details_t(const name& c): code(c) {}

    uint64_t    primary_key()const { return code.value; }
    uint64_t    scope() const { return 0; }
    uint64_t    by_creator() const { return creator.value; }
    checksum256 by_title() const { return HASH256(title); }

    typedef eosio::multi_index
    <"details"_n, details_t,
        indexed_by<"bycreator"_n, const_mem_fun<details_t, uint64_t, &details_t::by_creator>>,
        indexed_by<"bytitle"_n, const_mem_fun<details_t, checksum256, &details_t::by_title>>
    > idx_t;

    EOSLIB_SERIALIZE( details_t, (code)(type)(status)(creator)(title)(logo)(desc)(tags)(links)(strategy_id)(dapps) )

};




} //xdao