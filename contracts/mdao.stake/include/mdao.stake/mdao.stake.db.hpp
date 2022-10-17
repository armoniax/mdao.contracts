
#pragma once

#include <eosio/asset.hpp>
#include <eosio/singleton.hpp>
#include <eosio/privileged.hpp>
#include <eosio/name.hpp>
// #include <eosio.token/eosio.token.hpp>
#include <amax.ntoken/amax.ntoken.hpp>

#include <map>

using namespace eosio;

namespace mdao
{
    using namespace std;
    using namespace eosio;
    using namespace amax;

    struct [[eosio::table]] stake_global_t
    {
        name manager;
        set<name> supported_contracts;
        bool initialized = false;

        EOSLIB_SERIALIZE(stake_global_t, (id)(supported_contracts)(initialized));
        typedef eosio::singleton<"global"_n, stake_global_t> stake_global_singleton;
    };

    struct [[eosio::table]] dao_stake_t
    {
        name daocode;

        map<symbol, uint64_t> token_stake;
        map<uint64_t, uint64_t> nft_stake;
        uint32_t user_count;

        uint64_t primary_key() const { return daocode.value; }
        uint64_t scope() const { return 0; }

        dao_stake_t() {}
        dao_stake_t(const name& code): daocode(code) {}

        EOSLIB_SERIALIZE(dao_stake_t, (daocode)(token_stake)(user_count));
        typedef eosio::multi_index<"daostake"_n, dao_stake_t> idx_t;

    };
    
    uint128_t get_unionid(name account, name daocode) { return (uint128_t(account.value)<<64 | (uint128_t(daocode.value)<<64));}

    struct [[eosio::table]] user_stake_t
    {
        uint64_t id;
        name account;
        name daocode;
        map<symbol, uint64_t> token_stake;
        map<uint64_t, uint64_t> nft_stake;
        time_point_sec freeze_until;

        user_stake_t() {}
        user_stake_t(const name& code, const name& acc): account(acc), daocode(code) {}
        user_stake_t(const uint64_t &id, const name& code, const name& acc): id(id), daocode(code), account(account) {}

        uint64_t primary_key() const { return id; }
        uint64_t by_account() const { return account.value; }
        uint64_t by_daocode() const { return daocode.value; }
        uint128_t by_unionid() const { return mdao::get_unionid(account, daocode); }
        uint64_t scope() const { return 0; }

        EOSLIB_SERIALIZE(user_stake_t, (id)(account)(daocode)(token_stake)(nft_stake)(freeze_until))
    
        typedef eosio::multi_index<"usrstake"_n, user_stake_t,
            eosio::indexed_by<"userid"_n, const_mem_fun<user_stake_t, uint64_t, &user_stake_t::by_account>>,
            eosio::indexed_by<"daocodeid"_n, const_mem_fun<user_stake_t, uint64_t, &user_stake_t::by_daocode>>,
            eosio::indexed_by<"unionid"_n, const_mem_fun<user_stake_t, uint128_t, &user_stake_t::by_unionid>>>
            idx_t;
    };

} // amax