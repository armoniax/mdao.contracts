
#pragma once

#include <eosio/asset.hpp>
#include <eosio/singleton.hpp>
#include <eosio/privileged.hpp>
#include <eosio/name.hpp>
#include <amax.ntoken/amax.ntoken.hpp>

#include <map>

using namespace eosio;
#define STAKE_TBL [[eosio::table, eosio::contract("mdao.stake")]]
#define TG_TBL_NAME(name) [[eosio::table(name), eosio::contract("mdao.stake")]]

namespace mdao
{
    using namespace std;
    using namespace eosio;
    using namespace amax;

    struct TG_TBL_NAME("global") stake_global_t
    {
        set<name> managers;
        set<name> supported_tokens;
        bool initialized = false;

        EOSLIB_SERIALIZE(stake_global_t, (managers)(supported_tokens)(initialized));
        typedef eosio::singleton<"global"_n, stake_global_t> stake_global_singleton;
    };

    struct STAKE_TBL dao_stake_t
    {
        name daocode;

        map<extended_symbol, int64_t> tokens_stake;
        map<extended_nsymbol, int64_t> nfts_stake;
        uint32_t user_count;

        uint64_t primary_key() const { return daocode.value; }
        uint64_t scope() const { return 0; }

        dao_stake_t() {}
        dao_stake_t(const name& code): daocode(code) {}

        EOSLIB_SERIALIZE(dao_stake_t, (daocode)(tokens_stake)(nfts_stake)(user_count));
        typedef eosio::multi_index<"daostake"_n, dao_stake_t> idx_t;

    };
    
    uint128_t get_unionid(name account, name daocode) { return (uint128_t(account.value)<<64 | daocode.value);}

    struct STAKE_TBL user_stake_t
    {
        uint64_t id;
        name account;
        name daocode;
        map<extended_symbol, int64_t> tokens_stake;
        map<extended_nsymbol, int64_t> nfts_stake;
        time_point_sec freeze_until;

        user_stake_t() {}
        user_stake_t(const uint64_t &id): id(id) {}
        user_stake_t(const name& code, const name& acc): account(acc), daocode(code) {}
        user_stake_t(const uint64_t &id, const name& code, const name& acc): id(id), daocode(code), account(acc) {}

        uint64_t primary_key() const { return id; }
        uint64_t by_account() const { return account.value; }
        uint64_t by_daocode() const { return daocode.value; }
        uint128_t by_unionid() const { return mdao::get_unionid(account, daocode); }
        uint64_t scope() const { return 0; }

        EOSLIB_SERIALIZE(user_stake_t, (id)(account)(daocode)(tokens_stake)(nfts_stake)(freeze_until))
    
        typedef eosio::multi_index<"usrstake"_n, user_stake_t,
            eosio::indexed_by<"userid"_n, const_mem_fun<user_stake_t, uint64_t, &user_stake_t::by_account>>,
            eosio::indexed_by<"daocodeid"_n, const_mem_fun<user_stake_t, uint64_t, &user_stake_t::by_daocode>>,
            eosio::indexed_by<"unionid"_n, const_mem_fun<user_stake_t, uint128_t, &user_stake_t::by_unionid>>>
            idx_t;
    };

} // amax
