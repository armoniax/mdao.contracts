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

#define HASH256(str) sha256(const_cast<char*>(str.c_str()), str.size());


static constexpr uint64_t   seconds_per_day       = 24 * 3600;

namespace xdao {

using namespace std;
using namespace eosio;
// using namespace wasm;


struct app_info {
    string latest_app_version;
    string android_apk_download_url; //ipfs url
    string upgrade_log;
    bool force_upgrade;
    friend bool operator < (const app_info& symbol1, const app_info& appinfo2);
    EOSLIB_SERIALIZE(app_info, (latest_app_version)(android_apk_download_url)(upgrade_log)(force_upgrade) )
};

bool operator < (const app_info& appinfo1, const app_info& appinfo2) {
    return appinfo1.latest_app_version < appinfo2.latest_app_version;
}

struct [[eosio::table("global"), eosio::contract("xdao.conf")]] global_t {
    app_info          appinfo;
    name              status;
    name              fee_taker;
    asset             dao_upg_fee;
    uint16_t          amc_token_seats_max;
    uint16_t          evm_token_seats_max;
    uint16_t          dapp_seats_max;
    map<name, name>   managers;

    EOSLIB_SERIALIZE( global_t, (appinfo)(status)(fee_taker)(dao_upg_fee)(amc_token_seats_max)(evm_token_seats_max)(dapp_seats_max)(managers) )
};


typedef eosio::singleton< "global"_n, global_t > global_singleton;

struct stg_t {
    uint64_t id;
    uint64_t    primary_key()const { return id; }
    stg_t() {}
    stg_t(const uint64_t& sid): id(sid) {}
    uint64_t    scope() const { return 0; }
    typedef eosio::multi_index <"stgs"_n, stg_t> idx_t;
    EOSLIB_SERIALIZE( stg_t, (id))

};

struct gov_t {
    uint64_t id;
    uint64_t    primary_key()const { return id; }
    gov_t() {}
    gov_t(const uint64_t& gid): id(gid) {}
    uint64_t    scope() const { return 0; }
    typedef eosio::multi_index <"govs"_n, gov_t> idx_t;
    EOSLIB_SERIALIZE( gov_t, (id))

};

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
