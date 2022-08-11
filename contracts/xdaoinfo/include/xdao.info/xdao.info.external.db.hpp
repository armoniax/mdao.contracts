 #pragma once

#include <eosio/asset.hpp>
#include <eosio/privileged.hpp>
#include <eosio/system.hpp>
#include <eosio/time.hpp>

#include <deque>
#include <optional>
#include <string>
#include <map>
#include <set>
#include <type_traits>

namespace xdao {

using namespace std;
using namespace eosio;

struct wallet_t {
    uint64_t                id;
    string                  title;
    uint32_t                mulsign_m;
    uint32_t                mulsign_n;      // m <= n
    map<name, uint32_t>     mulsigners;     // mulsigner : weight
    map<extended_symbol, int64_t>    assets;         // symb@bank_contract  : amount
    uint64_t                proposal_expiry_sec = seconds_per_day * 7;
    name                    creator;
    time_point_sec          created_at;
    time_point_sec          updated_at;

    uint64_t primary_key()const { return id; }

    wallet_t() {}
    wallet_t(const uint64_t& wid): id(wid) {}
    uint64_t    scope() const { return 0; }

    uint64_t by_creator()const { return creator.value; }
    checksum256 by_title()const { return HASH256(title); }

    EOSLIB_SERIALIZE( wallet_t, (id)(title)(mulsign_m)(mulsign_n)(mulsigners)(assets)(proposal_expiry_sec)
                                (creator)(created_at)(updated_at) )

    typedef eosio::multi_index
    < "wallets"_n,  wallet_t,
        indexed_by<"creatoridx"_n, const_mem_fun<wallet_t, uint64_t, &wallet_t::by_creator> >,
        indexed_by<"titleidx"_n, const_mem_fun<wallet_t, checksum256, &wallet_t::by_title> >
    > idx_t;
};

struct  account {
    asset    balance;
    bool     can_send = false;
    bool     can_recv = false;
    uint64_t primary_key()const { return balance.symbol.code().raw(); }
    typedef eosio::multi_index< "accounts"_n, account > idx_t;

};

struct  currency_stats {
    asset    supply;
    asset    max_supply;
    name     issuer;

    uint64_t primary_key()const { return supply.symbol.code().raw(); }
    typedef eosio::multi_index< "stat"_n, currency_stats > idx_t;

};

} // xdao
