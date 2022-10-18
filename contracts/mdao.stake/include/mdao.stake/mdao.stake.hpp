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

enum class stake_err : uint8_t
{
    UNDEFINED = 1,
    STAKE_NOT_FOUND = 2,
    INVALID_PARAMS = 3,
    DAO_NOT_FOUND = 4,
    STILL_IN_LOCK = 5,
    UNLOCK_OVERFLOW = 6,
    INITIALIZED = 7,
    UNINITIALIZED = 8,
    UNSUPPORT_CONTRACT = 9,
    NOT_POSITIVE = 10,
};

class [[eosio::contract]] mdaostake : public contract
{
private:
    dbc _db;
    stake_global_t _gstate;
    stake_global_t::stake_global_singleton _global;

public:
    using contract::contract;
    mdaostake(name receiver, name code, datastream<const char *> ds) : contract(receiver, code, ds), _db(_self), _global(_self,_self.value) {
        if (_global.exists()) _gstate = _global.get();
        else _gstate = stake_global_t{};
    }
    
    ACTION init( const name& manager, set<name>supported_contracts );

    /**
     * stake token method
     * @from 
     * @to 
     * @quantity
     * @memo "daocode"
    */
    [[eosio::on_notify("amax.token::transfer")]]
    ACTION staketoken(const name &from, const name &to, const asset &quantity, const string &memo);

    ACTION unlocktoken(const name &account, const name &daocode, const vector<asset> &tokens);

    /**
     * stake token method
     * @from
     * @to
     * @quantity
     * @memo "daocode"
     */
    [[eosio::on_notify("amax.ntoken::transfer")]]
    ACTION stakenft(name from, name to, vector<nasset> &assets, string memo);
    
    ACTION unlocknft(const name &account, const name &daocode, const vector<nasset> &nfts);

    ACTION extendlock(const name &account, const name &daocode, const uint64_t &locktime);
};