
#pragma once

#include <eosio/asset.hpp>
#include <eosio/singleton.hpp>
#include <eosio/privileged.hpp>
#include <eosio/name.hpp>
#include <map>
#include <set>

using namespace eosio;

#define HASH256(str) sha256(const_cast<char*>(str.c_str()), str.size());
static constexpr uint64_t   seconds_per_day       = 24 * 3600;

namespace mdao {

#define INFO_TG_TBL [[eosio::table, eosio::contract("mdaoinfotest")]]

struct INFO_TG_TBL details_t {
    name                    code;
    name                    status;
    name                    creator;
    string                  title;
    string                  logo;
    string                  desc;
    set<string>             tags;
    map<name, string>       links;
    map<name, uint64_t>     strategys;
    set<app_info>           dapps;
    string                  group_id;
    time_point_sec          created_at;
    vector<extended_symbol> tokens;
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
// 
    EOSLIB_SERIALIZE( details_t, (code)(status)(creator)(title)(logo)
                        (desc)(tags)(links)(strategys)(dapps)(group_id)(created_at)(tokens) )

};

struct [[eosio::table]] account {
    asset    balance;

    uint64_t primary_key()const { return balance.symbol.code().raw(); }
};

typedef eosio::multi_index< "accounts"_n, account > accounts;


 
struct [[eosio::table]] currency_stats
{
    asset supply;
    asset max_supply;
    name issuer;
    bool is_paused = false;
    name fee_receiver;
    uint16_t gas_ratio = 0;
    uint16_t fee_ratio = 0;         // fee ratio, boost 10000
    asset min_fee_quantity;         // min fee quantity
    std::string fullname;

    uint64_t primary_key() const { return supply.symbol.code().raw(); }
};

typedef eosio::multi_index<"stat"_n, currency_stats> stats;


} //mdao