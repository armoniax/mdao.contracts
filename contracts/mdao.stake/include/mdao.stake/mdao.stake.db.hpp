
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

    struct [[eosio::table]] dao_stake_t
    {
        name daocode;

        map<symbol, asset> token_stake;
        // map<name, uint64_t> nft_stake;
        uint32_t user_count;

        uint64_t primary_key() const { return daocode.value; }

        dao_stake_t() {}
        // dao_stake_t(const name &c) : daocode(c) {}

        EOSLIB_SERIALIZE(dao_stake_t, (daocode)(token_stake)(user_count));

    };
    
    typedef eosio::multi_index<"daostake"_n, dao_stake_t> dao_stake_idx_t;

    struct [[eosio::table]] user_stake_t
    {
        uint64_t id;
        name account;
        name daocode;
        map<symbol, asset> token_stake;
        // map<name, uint64_t> nft_stake;
        time_point_sec freeze_until;

        user_stake_t() {}
        // user_stake_t(const uint64_t& pid): id(pid) {}

        uint64_t primary_key() const { return id; }
        uint64_t by_account() const { return account.value; }
        uint64_t by_daocode() const { return daocode.value; }

        EOSLIB_SERIALIZE(user_stake_t, (id)(account)(daocode)(token_stake)(freeze_until))
    };

    typedef eosio::multi_index<"usrstake"_n, user_stake_t,
        eosio::indexed_by<"userid"_n, const_mem_fun<user_stake_t, uint64_t, &user_stake_t::by_account>>,
        eosio::indexed_by<"daocodeid"_n, const_mem_fun<user_stake_t, uint64_t, &user_stake_t::by_daocode>>>
        user_stake_idx_t;


    uint64_t auto_hash_key(name account, name daocode) { return account.value ^ daocode.value; }
    // struct [[eosio::table]] nft_ownership
    // {
    //     name symbol;
    //     uint32_t index;
    //     name owner;

    //     nft_ownership() {}

    //     typedef eosio::multi_index<"nft_ownership"_n, nft_ownership> idx_t;
    // };

} // amax