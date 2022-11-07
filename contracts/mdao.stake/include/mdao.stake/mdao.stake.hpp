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
    NO_PERMISSION = 11,
    unstake_OVERFLOW = 12
};
 #define EXTEND_LOCK(bank, manager, id, locktime) \
{ action(permission_level{get_self(), "active"_n }, bank, "extendlock"_n, std::make_tuple( manager, id, locktime )).send(); }

class [[eosio::contract("mdao.stake")]] mdaostake : public contract
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
    
    ACTION init( const set<name>& managers, const set<name>&supported_contracts );



    /**
     * stake token method
     * @from
     * @to
     * @quantity
     * @memo "daocode"
     */
    [[eosio::on_notify("amax.ntoken::transfer")]]
    void stakenft(name from, name to, vector<nasset> &assets, string memo);

    
    /**
     * stake token method
     * @from 
     * @to 
     * @quantity
     * @memo "daocode"
    */
    [[eosio::on_notify("*::transfer")]]
    void staketoken(const name &from, const name &to, const asset &quantity, const string &memo);

    ACTION unstaketoken(const uint64_t &id, const vector<extended_asset> &tokens);

    

    ACTION unstakenft(const uint64_t &id, const vector<extended_nasset> &nfts);

    ACTION extendlock(const name &manager, uint64_t &id, const uint32_t &locktime);

    static map<extended_nsymbol, int64_t> get_user_staked_nfts( const name& contract_account, const name& owner, const name& dao_code){
        user_stake_t::idx_t user_stake(contract_account, contract_account.value); 
        auto user_stake_index = user_stake.get_index<"unionid"_n>(); 
        auto user_stake_iter = user_stake_index.find(mdao::get_unionid(owner, dao_code)); 
        if(user_stake_iter != user_stake_index.end()) return user_stake_iter->nfts_stake;
        map<extended_nsymbol, int64_t> empty_map;
        return empty_map;
    }

    static map<extended_symbol, int64_t> get_user_staked_tokens( const name& contract_account, const name& owner, const name& dao_code){
        user_stake_t::idx_t user_stake(contract_account, contract_account.value); 
        auto user_stake_index = user_stake.get_index<"unionid"_n>(); 
        auto user_stake_iter = user_stake_index.find(mdao::get_unionid(owner, dao_code)); 
        if(user_stake_iter != user_stake_index.end()) return user_stake_iter->tokens_stake;
        map<extended_symbol, int64_t> empty_map;
        return empty_map;
    }
};
