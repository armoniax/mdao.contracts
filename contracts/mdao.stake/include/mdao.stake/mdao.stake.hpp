#pragma once

#include <eosio/asset.hpp>
#include <eosio/eosio.hpp>
#include <eosio/singleton.hpp>
#include <eosio/time.hpp>
#include "mdao.stake.db.hpp"
#include <thirdparty/wasm_db.hpp>

using namespace eosio;
using namespace wasm::db;
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

class [[eosio::contract("mdao.stake")]] mdaostake : public contract
{

    // using conf_t = mdao::conf_global_t;
    // using conf_table_t = mdao::conf_global_singleton;

private:
    dbc _db;
    // std::unique_ptr<conf_table_t> _conf_tbl_ptr;
    // std::unique_ptr<conf_t> _conf_ptr;

    // const conf_t &_conf();

public:
    using contract::contract;
    mdaostake(name receiver, name code, datastream<const char *> ds) : _db(_self), contract(receiver, code, ds) {}

    ACTION stakeToken(const name &account, const name &daocode, const map<name, asset> &tokens, const uint64_t &locktime);

    // ACTION stakeNft(const name &account, const name &daocode, const map<name, uint64_t> &nfts, const uint64_t &locktime);

    ACTION withdrawToken(const name &account, const name &daocode, const map<name, asset> &tokens);

    // ACTION withdrawNft(const name &account, const name &daocode, const map<name, uint64_t> &nfts);

};