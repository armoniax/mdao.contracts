
#pragma once

#include <eosio/asset.hpp>
#include <eosio/singleton.hpp>
#include <eosio/privileged.hpp>
#include <eosio/name.hpp>

#include <map>

using namespace eosio;


namespace mdao {

using namespace eosio;
using namespace std;
using std::string;
#define SYMBOL(sym_code, precision) symbol(symbol_code(sym_code), precision)


static constexpr symbol AMAX_SYMBOL            = SYMBOL("AMAX", 8);
static constexpr uint32_t seconds_per_day      = 24 * 3600;
static constexpr uint32_t seconds_per_month    = 30 * 24 * 3600;
static constexpr uint16_t TEN_THOUSAND         = 10000;
static constexpr name      SPLIT_ACCOUNT        { "amax.split"_n };
static constexpr name      MDAO_INFO_ACCOUNT        { "mdao.info"_n };

#define MEEING_TBL struct [[eosio::table, eosio::contract("mdao.meeting")]]
#define MEEING_TBL_NAME(name) struct [[eosio::table(name), eosio::contract("mdao.meeting")]]

namespace status {
    static constexpr name ENABLED           = "enabled"_n;
    static constexpr name DISABLED          = "disabled"_n;
};

namespace meeting_action_name {
    static constexpr uint64_t DAO                    = "dao"_n.value;
};


MEEING_TBL_NAME("global") global_t {
    name                    admin;
    bool                    running      = true;
    uint16_t                split_id;
    extended_asset          quantity = extended_asset(1000000000,extended_symbol(AMAX_SYMBOL,"amax.token"_n));
    name                    receiver;
    uint64_t                reserved0;
    typedef eosio::singleton< "global"_n, global_t > tbl_t;
};

// scope : self
MEEING_TBL meeting_t{
    name                        dao_code;
    string                      group_id;       // PK
    name                        creator;
    name                        status;
    uint64_t                    reserved0;
    time_point_sec              created_at;
    time_point_sec              updated_at;
    time_point_sec              expired_at;
    
    uint64_t primary_key()const { return dao_code.value; }

    typedef eosio::multi_index< "meetings"_n, meeting_t > tbl_t;
};


} //amax