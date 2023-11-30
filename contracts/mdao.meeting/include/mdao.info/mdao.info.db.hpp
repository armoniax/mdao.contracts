
#pragma once

#include <eosio/asset.hpp>
#include <eosio/singleton.hpp>
#include <eosio/privileged.hpp>
#include <amax.ntoken/amax.ntoken.db.hpp>
#include <eosio/name.hpp>
#include <map>
#include <set>

using namespace eosio;
using namespace amax;

// #define HASH256(str) sha256(const_cast<char*>(str.c_str()), str.size());

namespace mdao {

#define INFO_TG_TBL [[eosio::table, eosio::contract("mdao.info")]]


struct app_info {
    name app_name;
    string app_version;
    string url;
    string logo;

    friend bool operator < (const app_info& appinfo1, const app_info& appinfo2);

    EOSLIB_SERIALIZE(app_info, (app_name)(app_version)(url)(logo) )
};
struct tags_info {
    vector<string> tags;
    EOSLIB_SERIALIZE(tags_info, (tags) )
};
struct INFO_TG_TBL dao_info_t {
    name                        dao_code;
    string                      title;
    string                      logo;
    string                      desc;
    map<name, tags_info>        tags;
    map<name, string>           resource_links;
    set<app_info>               dapps;
    string                      group_id;
    extended_symbol             token;
    extended_nsymbol            ntoken;
    name                        status;
    name                        creator;
    time_point_sec              created_at;
    string                      memo;

    dao_info_t() {}
    dao_info_t(const name& c): dao_code(c) {}

    uint64_t    primary_key()const { return dao_code.value; }
    uint64_t    scope() const { return 0; }
    uint64_t    by_creator() const { return creator.value; }
    checksum256 by_title() const { return HASH256(title); }

    typedef eosio::multi_index
    <"infos"_n, dao_info_t,
        indexed_by<"bycreator"_n, const_mem_fun<dao_info_t, uint64_t, &dao_info_t::by_creator>>,
        indexed_by<"bytitle"_n, const_mem_fun<dao_info_t, checksum256, &dao_info_t::by_title>>
    > idx_t;

    EOSLIB_SERIALIZE( dao_info_t, (dao_code)(title)(logo)(desc)(tags)(resource_links)(dapps)(group_id)
                                    (token)(ntoken)(status)(creator)(created_at)(memo) )

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

    uint64_t primary_key() const { return supply.symbol.code().raw(); }
};

typedef eosio::multi_index<"stat"_n, currency_stats> stats;


} //mdao