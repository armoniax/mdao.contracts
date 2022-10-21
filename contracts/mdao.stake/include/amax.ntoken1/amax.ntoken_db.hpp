#pragma once

#include <eosio/asset.hpp>
#include <eosio/privileged.hpp>
#include <eosio/singleton.hpp>
#include <eosio/system.hpp>
#include <eosio/time.hpp>

// #include <deque>
#include <optional>
#include <string>
#include <map>
#include <set>
#include <type_traits>
#include "./amax.nasset.hpp"
#include "./amax.nsymbol.hpp"

namespace amax {

using namespace std;
using namespace eosio;

#define HASH256(str) sha256(const_cast<char*>(str.c_str()), str.size())
#define TBL struct [[eosio::table, eosio::contract("amax.ntoken")]]
#define NTBL(name) struct [[eosio::table(name), eosio::contract("amax.ntoken")]]

TBL nstats_t {
    nasset          supply;
    nasset          max_supply;     // 1 means NFT-721 type
    string          token_uri;      // globally unique uri for token metadata { image, desc,..etc }
    name            ipowner;        // who owns the IP
    name            notary;         // who notarized the IP authenticity and owership
    name            issuer;         // who created/uploaded/issued this NFT
    time_point_sec  issued_at;
    time_point_sec  notarized_at;
    bool            paused;

    nstats_t() {};
    nstats_t(const uint64_t& id): supply(id) {};
    nstats_t(const uint64_t& id, const uint64_t& pid): supply(id, pid) {};
    nstats_t(const uint64_t& id, const uint64_t& pid, const int64_t& am): supply(id, pid, am) {};
    
    uint64_t primary_key()const     { return supply.symbol.id; } // must use id to keep available_primary_key increase consistenly
    uint64_t by_parent_id()const    { return supply.symbol.parent_id; }
    uint64_t by_ipowner()const      { return ipowner.value; }
    uint64_t by_issuer()const       { return issuer.value; }
    uint128_t by_issuer_created()const { return (uint128_t) issuer.value << 64 | (uint128_t) issued_at.sec_since_epoch(); }
    checksum256 by_token_uri()const { return HASH256(token_uri); } // unique index

    typedef eosio::multi_index
    < "tokenstats"_n,  nstats_t,
        indexed_by<"parentidx"_n,       const_mem_fun<nstats_t, uint64_t, &nstats_t::by_parent_id> >,
        indexed_by<"ipowneridx"_n,      const_mem_fun<nstats_t, uint64_t, &nstats_t::by_ipowner> >,
        indexed_by<"issueridx"_n,       const_mem_fun<nstats_t, uint64_t, &nstats_t::by_issuer> >,
        indexed_by<"issuercreate"_n,    const_mem_fun<nstats_t, uint128_t, &nstats_t::by_issuer_created> >,
        indexed_by<"tokenuriidx"_n,     const_mem_fun<nstats_t, checksum256, &nstats_t::by_token_uri> >
    > idx_t;

    EOSLIB_SERIALIZE(nstats_t,  (supply)(max_supply)(token_uri)
                                (ipowner)(notary)(issuer)(issued_at)(notarized_at)(paused) )
};


///Scope: owner's account
TBL account_t {
    nasset      balance;
    bool        paused = false;   //if true, it can no longer be transferred

    account_t() {}
    account_t(const nasset& asset): balance(asset) {}

    uint64_t primary_key()const { return balance.symbol.raw(); }

    EOSLIB_SERIALIZE(account_t, (balance)(paused) )

    typedef eosio::multi_index< "accounts"_n, account_t > idx_t;
};

} //namespace amax