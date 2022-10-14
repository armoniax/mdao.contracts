#pragma once

#include <eosio/asset.hpp>
#include <eosio/eosio.hpp>
#include <eosio/singleton.hpp>
#include <eosio/time.hpp>
#include "mdao.stake.db.hpp"
#include <thirdparty/wasm_db.hpp>


using namespace eosio;
using namespace mdao;
using namespace std;
using namespace wasm::db;

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
private:
    dbc _db;

public:
    using contract::contract;
    mdaostake(name receiver, name code, datastream<const char *> ds) : contract(receiver, code, ds), _db(_self) {}

    ACTION staketoken(const name &account, const name &daocode, const vector<asset> &tokens, const uint64_t &locktime);

    ACTION unlocktoken(const name &account, const name &daocode, const vector<asset> &tokens);

    ACTION stakenft(const name &account, const name &daocode, const vector<nasset> &nfts, const uint64_t &locktime);

    ACTION unlocknft(const name &account, const name &daocode, const vector<nasset> &nfts);

    ACTION extendlock(const name &account, const name &daocode, const uint64_t &locktime);
};