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

static constexpr symbol AM_SYMBOL = symbol(symbol_code("AMAX"), 8);

// static constexpr name AMAX_TOKEN{"amax.token"_n};
// static constexpr name MDAO_CONF{"mdao.conf"_n};
// static constexpr name MDAO_STG{"mdao.stg"_n};
// static constexpr name MDAO_GOV{"mdao.gov"_n};
// static constexpr name MDAO_VOTE{"mdao.vote"_n};
// static constexpr name MDAO_STAKE{"mdao.stake"_n};
// static constexpr name AMAX_MULSIGN{"amax.mulsign"_n};

enum class stake_err : uint8_t
{
    ACCOUNT_NOT_EXITS = 1
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

    // [[eosio::action]]
    // void createdao(const name& owner,const name& code, const string& title,
    //                        const string& logo, const string& desc,
    //                        const set<string>& tags, const map<name, string>& links);

    [[eosio::action]] ACTION creategov(const name &creator, const name &daocode, const string &title,
                                       const string &desc, const set<uint64_t> &votestg, const uint32_t minvotes);

    [[eosio::action]] ACTION cancel(const name &owner, const name &daocode);

    [[eosio::action]] ACTION setpropstg(const name &owner, const name &daocode,
                                        set<name> proposers, set<uint64_t> proposestg);

    [[eosio::action]] ACTION createprop(const name &owner, const name &creator, const name &daocode,
                                        const uint64_t &stgid, const string &name,
                                        const string &desc, const uint32_t &votes);

    [[eosio::action]] ACTION excuteprop(const name &owner, const name &code, const uint64_t &proposeid);
};