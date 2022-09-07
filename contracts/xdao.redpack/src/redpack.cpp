
#include <amax.token.hpp>
#include "redpack.hpp"
#include "utils.hpp"
#include <algorithm>
#include <chrono>
#include <eosio/transaction.hpp>
#include <eosio/crypto.hpp>

using std::chrono::system_clock;
using namespace wasm;

static constexpr eosio::name active_permission{"active"_n};

// transfer out from contract self
#define TRANSFER_OUT(bank, to, quantity, memo) \
    { action(permission_level{get_self(), "active"_n }, bank, "transfer"_n, std::make_tuple( _self, to, quantity, memo )).send(); }

inline int64_t get_precision(const symbol &s) {
    int64_t digit = s.precision();
    CHECKC(digit >= 0 && digit <= 18, err::SYMBOL_MISMATCH, "precision digit " + std::to_string(digit) + " should be in range[0,18]");
    return calc_precision(digit);
}

inline int64_t get_precision(const asset &a) {
    return get_precision(a.symbol);
}

//issue-in op: transfer tokens to the contract and lock them according to the given plan
void redpack::ontransfer( name from, name to, asset quantity, string memo )
{
    if (from == _self || to != _self) return;

	CHECKC( quantity.amount > 0, err::NOT_POSITIVE, "quantity must be positive" )

    //memo params format:
    //${pwhash} : count : type : code
    auto parts = split( memo, ":" );
    CHECKC( parts.size() == 4, err::INVALID_FORMAT,"Expected format 'pwhash : count : type : code'" );

    auto code = name(parts[3]);
    redpack_t redpack(code);
    CHECKC( !_db.get(redpack), err::RED_PACK_EXIST, "code is already exists" );

    auto count = stoi(string(parts[1]));

    auto type = stoi(string(parts[2]));
    CHECKC( (redpack_type)type == redpack_type::RANDOM || (redpack_type)type == redpack_type::MEAN, err::TYPE_INVALID, "redpack type invalid" );

    auto fee_info = fee_t(quantity.symbol);
    CHECKC( _db.get(fee_info), err::FEE_NOT_FOUND, "fee not found" );

    asset fee = _calc_fee( fee_info.fee, count );
    CHECKC( fee < quantity, err::QUANTITY_NOT_ENOUGH , "not enough " );

    asset total_quantity = quantity - fee;
    CHECKC( (total_quantity/count).amount >= power10(quantity.symbol.precision()-fee_info.min_unit), err::QUANTITY_NOT_ENOUGH , "not enough " );

    redpack_t::idx_t redpacks( _self, _self.value );
    // auto id = redpacks.available_primary_key();
    redpacks.emplace( _self, [&]( auto& row ) {
        row.code 					    = code;
        row.sender 			            = from;
        row.pw_hash                     = string( parts[0] );
        row.total_quantity              = total_quantity;
        row.fee                         = fee;
        row.receiver_count		        = count;
        row.remain_quantity		        = total_quantity;
        row.remain_count	            = count;
        row.status			            = redpack_status::CREATED;
        row.type			            = type;
        row.created_at                  = time_point_sec( current_time_point() );
        row.updated_at                  = time_point_sec( current_time_point() );
   });

}
void redpack::claim( const name& claimer, const name& code, const string& pwhash )
{
    require_auth( _gstate.tg_admin );

    redpack_t redpack(code);
    CHECKC( _db.get(redpack), err::RECORD_NO_FOUND, "redpack not found" );
    CHECKC( redpack.pw_hash == pwhash, err::PWHASH_INVALID, "incorrect password" );
    CHECKC( redpack.status == redpack_status::CREATED, err::EXPIRED, "redpack has expired" );
    CHECKC( (redpack_type)redpack.type == redpack_type::RANDOM || (redpack_type)redpack.type == redpack_type::MEAN, err::TYPE_INVALID, "redpack type invalid" );

    claim_t::idx_t claims(_self, _self.value);
    auto claims_index = claims.get_index<"unionid"_n>();
    uint128_t sec_index = get_unionid(claimer, code.value);
    auto claims_iter = claims_index.find(sec_index);
    eosio::print(claims_iter->id);
    CHECKC( claims_iter == claims_index.end() ,err::NOT_REPEAT_RECEIVE, "Can't repeat to receive" );

    fee_t fee_info(redpack.total_quantity.symbol);
    CHECKC( _db.get(fee_info), err::FEE_NOT_FOUND, "fee not found" );
    asset redpack_quantity;

    switch((redpack_type)redpack.type){
        case redpack_type::RANDOM  :
            redpack_quantity = _calc_red_amt(redpack,fee_info.min_unit);
            break;
        case redpack_type::MEAN  :
            redpack_quantity = redpack.remain_count == 1 ? redpack.remain_quantity : redpack.total_quantity/redpack.receiver_count;
            break;
    }
    TRANSFER_OUT(fee_info.contract_name, claimer, redpack_quantity, string("red pack transfer"));

    redpack.remain_count--;
    redpack.remain_quantity-=redpack_quantity;
    redpack.updated_at = time_point_sec( current_time_point() );
    if(redpack.remain_count == 0){
        redpack.status = redpack_status::FINISHED;
    }
    _db.set(redpack);

    auto id = claims.available_primary_key();
    claims.emplace( _self, [&]( auto& row ) {
        row.id                  = id;
        row.red_pack_code 	    = code;
        row.sender              = redpack.sender;
        row.receiver            = claimer;
        row.quantity            = redpack_quantity;
        row.claimed_at		    = time_point_sec( current_time_point() );
   });

}

void redpack::cancel( const name& code )
{
    require_auth( _gstate.tg_admin );
    redpack_t redpack(code);
    CHECKC( _db.get(redpack), err::RECORD_NO_FOUND, "redpack not found" );
    CHECKC( redpack.status == redpack_status::CREATED, err::EXPIRED, "redpack has expired" );
    CHECKC( current_time_point() > redpack.created_at + eosio::hours(_gstate.expire_hours), err::NOT_EXPIRED, "expiration date is not reached" );

    fee_t fee_info(redpack.total_quantity.symbol);
    CHECKC( _db.get(fee_info), err::FEE_NOT_FOUND, "fee not found" );
    asset cancelamt = redpack.remain_quantity + fee_info.fee * redpack.remain_count;
    TRANSFER_OUT(fee_info.contract_name, redpack.sender, cancelamt, string("red pack cancel transfer"));
    redpack.status = redpack_status::CANCELLED;
    _db.set(redpack);

}

void redpack::addfee( const asset& fee, const name& contract, const uint16_t& min_unit)
{
    require_auth( _self );
    CHECKC( fee.amount >= 0, err::FEE_NOT_POSITIVE, "fee must be positive" );
    CHECKC( get_precision(fee) >= power10(min_unit), err::MIN_UNIT_INVALID, "min unit not greater than coin precision" );

    auto fee_info = fee_t(fee.symbol);
    fee_info.fee = fee;
    fee_info.contract_name = contract;
    fee_info.min_unit = min_unit;
    _db.set( fee_info );
}

void redpack::delfee( const symbol& coin )
{
    require_auth( _self );
    auto fee_info = fee_t(coin);
    CHECKC( _db.get(fee_info), err::FEE_NOT_FOUND, "coin not found" );

    _db.del( fee_info );
}

void redpack::setconf(const name& admin, const uint16_t& hours)
{
    require_auth( _self );
    CHECKC( is_account(admin), err::ACCOUNT_INVALID, "account invalid" );
    CHECKC( hours > 0, err::VAILD_TIME_INVALID, "valid time must be positive" );

    _gstate.tg_admin = admin;
    _gstate.expire_hours = hours;

}

void redpack::delredpacks(name& code){
    require_auth( _self );

    redpack_t::idx_t redpacks(_self, _self.value);
    auto redpack_itr = redpacks.find(code.value);
    if( redpack_itr !=  redpacks.end()){
        redpacks.erase(redpack_itr);
    }

}

asset redpack::_calc_fee(const asset& fee, const uint64_t count) {
    // calc order quantity value by price
    auto value = multiply<uint64_t>(fee.amount, count);

    return asset(value, fee.symbol);
}

asset redpack::_calc_red_amt(const redpack_t& redpack,const uint16_t& min_unit) {
    // calc order quantity value by price
    if( redpack.remain_count == 1 ){
        return redpack.remain_quantity;

    }else{
        uint64_t quantity = redpack.remain_quantity.amount / redpack.remain_count * 2;
        return asset(rand(asset(quantity,redpack.remain_quantity.symbol),min_unit), redpack.remain_quantity.symbol);

    }
}

uint64_t redpack::rand(asset max_quantity,  uint16_t min_unit) {
    auto mixedBlock = tapos_block_prefix() * tapos_block_num();
    const char *mixedChar = reinterpret_cast<const char *>(&mixedBlock);
    auto hash = sha256( (char *)mixedChar, sizeof(mixedChar));
    int64_t min_unit_throot = power10(min_unit);

    auto r1 = (uint64_t)hash.data()[0];
    float rand = 1/min_unit_throot+r1 % 100 / 100.00;
    int64_t round_throot = power10(max_quantity.symbol.precision() - min_unit);

    uint64_t rand_value = (uint64_t)(max_quantity.amount * rand) / round_throot * round_throot;
    uint64_t min_value = get_precision(max_quantity) / min_unit_throot;

    return rand_value < min_value ? min_value : rand_value;
}



