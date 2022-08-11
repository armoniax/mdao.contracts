#include "redpackdb.hpp"
#include <wasm_db.hpp>

using namespace std;
using namespace wasm::db;

enum class err: uint8_t {
   INVALID_FORMAT       = 0,
   TYPE_INVALID         = 1,
   FEE_NOT_FOUND        = 2,
   QUANTITY_NOT_ENOUGH  = 3,
   NOT_POSITIVE         = 4,
   SYMBOL_MISMATCH      = 5,
   EXPIRED              = 6,
   PWHASH_INVALID       = 7,
   RECORD_NO_FOUND      = 8,
   NOT_REPEAT_RECEIVE   = 9,
   NOT_EXPIRED          = 10,
   ACCOUNT_INVALID      = 11,
   FEE_NOT_POSITIVE     = 12,
   VAILD_TIME_INVALID   = 13,
   MIN_UNIT_INVALID     = 14
};

enum class redpack_type: uint8_t {
   RANDOM       = 0,
   MEAN         = 1
};

class [[eosio::contract("xdao.redpack")]] redpack: public eosio::contract {
private:
    dbc                 _db;
    global_singleton    _global;
    global_t            _gstate;

public:
    using contract::contract;

    redpack(eosio::name receiver, eosio::name code, datastream<const char*> ds):
        _db(_self),
        contract(receiver, code, ds),
        _global(_self, _self.value)
    {
        _gstate = _global.exists() ? _global.get() : global_t{};
    }

    ~redpack() {
        _global.set( _gstate, get_self() );
    }


    //[[eosio::action]]
    [[eosio::on_notify("*::transfer")]] void ontransfer(name from, name to, asset quantity, string memo);

    [[eosio::action]] void claim( const name& claimer, const uint64_t& pack_id, const string& pwhash );

    [[eosio::action]] void cancel( const uint64_t& pack_id );

    [[eosio::action]] void addfee( const asset& fee, const name& contract, const uint16_t& min_unit);

    [[eosio::action]] void delfee( const symbol& coin );

    [[eosio::action]] void setconf( const name& admin, const uint16_t& hours );

    [[eosio::action]] void delredpacks( uint64_t& id );

    asset _calc_fee(const asset& fee, const uint64_t count);

    asset _calc_red_amt(const redpack_t& redpack,const uint16_t& min_unit);

    uint64_t rand(asset max_quantity,  uint16_t min_unit);
private:

}; //contract redpack