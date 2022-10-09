
#pragma once

#include <eosio/asset.hpp>
#include <eosio/singleton.hpp>
#include <eosio/privileged.hpp>
#include <eosio/name.hpp>

#include <map>

using namespace eosio;

namespace mdao
{

    using namespace std;
    using namespace eosio;

#define STAKE_TG_TBL [[eosio::table, eosio::contract("mdao.stake")]]

    struct STAKE_TG_TBL stake_t
    {
        name dao_code;

        map<name, uint64_t> stake_assets;
        map<name, uint64_t> stake_nfts;
        uint32_t user_count;

        uint64_t primary_key() const { return dao_code.value; }
        uint64_t scope() const { return 0; }

        stake_t() {}
        stake_t(const name &c) : dao_code(c) {}

        EOSLIB_SERIALIZE(stake_t, (dao_code)(stake_assets)(stake_nfts)(user_count))

        typedef eosio::multi_index<"stake"_n, stake_t> idx_t;
    };

    struct [[eosio::table]] user_stake
    {
        uint64_t id;
        name account;
        name dao_code;
        map<name, uint64_t> token_stake;
        map<name, uint64_t> nft_stake;
        time_point_sec freeze_until;

        user_stake() {}

        typedef eosio::multi_index<"user_stake"_n, user_stake> idx_t;
    };

    struct [[eosio::table]] nft_ownership
    {
        name symbol;
        uint32_t index;
        name owner;

        nft_ownership() {}

        typedef eosio::multi_index<"nft_ownership"_n, nft_ownership> idx_t;
    };

} // amax