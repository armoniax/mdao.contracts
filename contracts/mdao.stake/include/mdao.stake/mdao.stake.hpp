#pragma once

#include <eosio/asset.hpp>
#include <eosio/eosio.hpp>
#include <eosio/singleton.hpp>
#include <eosio/time.hpp>
#include "mdao.stake.db.hpp"

using namespace eosio;
using namespace mdao;
using namespace std;

// static constexpr name AMAX_TOKEN{"amax.token"_n};
// static constexpr name MDAO_CONF{"mdao.conf"_n};
// static constexpr name MDAO_STG{"mdao.stg"_n};
// static constexpr name MDAO_GOV{"mdao.gov"_n};
// static constexpr name MDAO_VOTE{"mdao.vote"_n};
// static constexpr name MDAO_STAKE{"mdao.stake"_n};
// static constexpr name AMAX_MULSIGN{"amax.mulsign"_n};

enum class stake_err : uint8_t
{
    UNDEFINED = 1
};

class [[eosio::contract]] mdaostake : public contract
{
// private:

public:
    using contract::contract;
    mdaostake(name receiver, name code, datastream<const char *> ds) : contract(receiver, code, ds), userstaketable(receiver, receiver.value), daostaketable(receiver, receiver.value) {}

    user_stake_idx_t userstaketable;

    dao_stake_idx_t daostaketable;

    ACTION staketoken(const name &account, const name &daocode, const map<symbol, asset> &tokens, const uint64_t &locktime);

    // ACTION stakenft(const name &account, const name &daocode, const map<name, nasset> &nfts, const uint64_t &locktime);

    ACTION unlocktoken(const name &account, const name &daocode, const map<symbol, asset> &tokens);

    // ACTION unlocknft(const name &account, const name &daocode, const map<name, nasset> &nfts);

};