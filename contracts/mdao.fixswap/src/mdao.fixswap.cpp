
#include "mdao.fixswap.hpp"
#include <aplink.farm/aplink.farm.hpp>

static constexpr eosio::name active_permission{"active"_n};

#define ALLOT(bank, land_id, customer, quantity, memo) \
    {	aplink::farm::allot_action act{ bank, { {_self, active_perm} } };\
			act.send( land_id, customer, quantity , memo );}

#define NOTIFICATION(account, info, memo) \
    {	mdao::fixswap::notification_action act{ _self, { {_self, active_permission} } };\
			act.send( account, info , memo );}

using namespace mdao;
using namespace std;
using namespace eosio;

// inline int64_t get_precision(const symbol &s) {
//     int64_t digit = s.precision();
//     CHECK(digit >= 0 && digit <= 18, "precision digit " + std::to_string(digit) + " should be in range[0,18]");
//     return calc_precision(digit);
// }

// inline int64_t get_precision(const asset &a) {
//     return get_precision(a.symbol);
// }

void fixswap::init(const name& admin, const name& fee_collector, const uint32_t& fee_ratio){
    require_auth(get_self());

    CHECKC( _gstate.status == swap_status_t::created, err::HAS_INITIALIZE, "cannot initialize again" )
    CHECKC( is_account(fee_collector), err::ACCOUNT_INVALID, "cannot found fee_collector" )
    CHECKC( is_account(admin), err::ACCOUNT_INVALID, "cannot found admin" )
    CHECKC( fee_ratio >= 0 && fee_ratio <= 1000, err::ACCOUNT_INVALID, "fee_ratio must be in range 0~10% (0~1000)" )

    _gstate.status = swap_status_t::initialized;
    _gstate.fee_collector = fee_collector;
    _gstate.admin = admin;
    _gstate.take_fee_ratio = fee_ratio;
    _gstate.make_fee_ratio = fee_ratio;
    _gstate.supported_tokens.insert("amax.token"_n);
    _gstate.supported_tokens.insert("amax.mtoken"_n);
    // _gstate.supported_tokens.insert("mdao.token"_n);
    _global.set( _gstate, get_self());
}

void fixswap::setfee(const name& fee_collector, const uint32_t& take_fee_ratio, const uint32_t& make_fee_ratio){
    require_auth(_gstate.admin);

    CHECKC( is_account(fee_collector), err::ACCOUNT_INVALID, "cannot found fee_collector" )
    CHECKC( take_fee_ratio >= 0 && take_fee_ratio <= 1000, err::NOT_POSITIVE, "take_fee_ratio must be in range 0~10% (0~1000)" )
    CHECKC( make_fee_ratio >= 0 && make_fee_ratio <= 1000, err::NOT_POSITIVE, "make_fee_ratio must be in range 0~10% (0~1000)" )

    _gstate.fee_collector = fee_collector;
    _gstate.take_fee_ratio = take_fee_ratio;
    _gstate.make_fee_ratio = make_fee_ratio;
    _global.set( _gstate, get_self());
}

void fixswap::setfarm(const uint64_t& farm_lease_id, const map<extended_symbol, int64_t>& farm_scales){
    require_auth(_gstate.admin);

    CHECKC( farm_lease_id >= 0, err::NOT_POSITIVE, "farm_lease_id cannot a negtive number" )

    _gstate.farm_lease_id = farm_lease_id;
    _gstate.farm_scales = farm_scales;
    _global.set( _gstate, get_self());
}

void fixswap::ontransfer(name from, name to, asset quantity, string memo)
{
    if(from == get_self() || to != get_self()) return;
    CHECKC( _gstate.status == swap_status_t::initialized, err::PAUSED, "contract is maintaining" )
    CHECKC( quantity.amount>0, err::NOT_POSITIVE, "swap quanity must be positive" )
    CHECKC( _gstate.supported_tokens.count(get_first_receiver()), err::SYMBOL_MISMATCH, "unsupport token contract" )

    vector<string_view> params = split(memo, ":");
    CHECKC( params.size() > 0, err::PARAM_ERROR, "unsupport memo" )

    if(params.size() == 6 && params[0] == "make"){
        auto swap_order = swap_t(name(params[1]));
        CHECKC( !_db.get(swap_order), err::RECORD_EXISTING, "repeat swap order" )
        swap_order.id = _gstate.swap_id++;
        swap_order.status = swap_status_t::matching;
        swap_order.maker = from;
        swap_order.created_at = current_time_point();
        swap_order.make_asset = extended_asset(quantity, get_first_receiver());

        if(params[2].length() > 0){
            name taker(params[2]);
            CHECKC( is_account(taker), err::ACCOUNT_INVALID, "cannot find taker account" )
            swap_order.taker = taker;
        }

        asset take_quant = asset_from_string(params[3]);
        CHECKC( take_quant.amount > 0, err::ACCOUNT_INVALID, "take quanity must be positive" )

        name take_contract = name(params[4]);
        CHECKC( is_account(take_contract), err::ACCOUNT_INVALID, "cannot find take quantity contract" )
        CHECKC( _gstate.supported_tokens.count(take_contract), err::SYMBOL_MISMATCH, "unsupport token contract" )

        swap_order.take_asset = extended_asset(take_quant, take_contract);
        if(params[5].length() > 0){
            swap_order.code = params[5];
        }
        swap_order.expired_at = time_point_sec(current_time_point()) + default_expired_secs;

        _db.set(swap_order, get_self());
        _global.set( _gstate, get_self());
    }
    else if(params.size() == 3 && params[0] == "take"){
        auto swap_orderno = name(params[1]);
        auto swap_order = swap_t(swap_orderno);
        CHECKC( _db.get(swap_order), err::RECORD_NOT_FOUND, "cannot found swap order" )
        CHECKC( time_point_sec(current_time_point()) < swap_order.expired_at , err::TIME_EXPIRED, "swap order expired")
        CHECKC( swap_order.status == swap_status_t::matching, err::STATUS_MISMATCH, "order status mismatched" )

        if(swap_order.taker != name(0)){
            CHECKC( swap_order.taker == from, err::NO_AUTH, "no auth to swap order" )
        }else{
            swap_order.taker = from;
        }

        if(swap_order.code.size() != 0){
            string data = string(params[2]);
            checksum256 digest = sha256(&data[0], data.size());
            auto bytes = digest.extract_as_byte_array();
            string hexstr = to_hex(bytes.data(), bytes.size());
            CHECKC( hexstr == swap_order.code, err::NO_AUTH, "auth failed: code mismatch: " + data + " | " + hexstr)
        }

        CHECKC( quantity == swap_order.take_asset.quantity, err::SYMBOL_MISMATCH, "swap quantity mismatch" )
        CHECKC( get_first_receiver() == swap_order.take_asset.contract, err::SYMBOL_MISMATCH, "quantity contract mismatch" )

        _transaction_transfer(swap_order.make_asset, swap_order.take_asset, swap_order.taker, swap_order.maker, swap_orderno);
        _reward_transfer(swap_order.make_asset, swap_order.take_asset, swap_order.taker, swap_order.maker, swap_orderno);

        _db.del(swap_order);
        notification(swap_orderno, swap_status_t::matched);

    }
}

void fixswap::_transaction_transfer(const extended_asset& make_asset, 
                                    const extended_asset& take_asset, 
                                    const name& maker, 
                                    const name& taker, 
                                    const name& order_no){

        asset make_fee = make_asset.quantity * _gstate.make_fee_ratio / percent_boost;
        asset take_fee = take_asset.quantity * _gstate.take_fee_ratio / percent_boost;

        TRANSFER(make_asset.contract, 
            taker, 
            make_asset.quantity - make_fee, 
            "Swap with " + make_asset.quantity.to_string());

        TRANSFER(take_asset.contract, 
            maker, 
            take_asset.quantity - take_fee, 
            "Swap with " + take_asset.quantity.to_string());

        if(make_fee.amount > 0){
            extended_asset extended_asset_maker_fee = extended_asset(make_fee, make_asset.contract);
            TRANSFER(extended_asset_maker_fee.contract, 
                _gstate.fee_collector, 
                extended_asset_maker_fee.quantity, 
                "Swap take fee of " + order_no.to_string());
        }

        if(take_fee.amount > 0){
            extended_asset extended_asset_taker_fee = extended_asset(take_fee, take_asset.contract);
            TRANSFER(extended_asset_taker_fee.contract, 
                _gstate.fee_collector, 
                extended_asset_taker_fee.quantity, 
                "Swap take fee of " + order_no.to_string());
        }
}

void fixswap::_reward_transfer(const extended_asset& make_asset, 
                                    const extended_asset& take_asset, 
                                    const name& maker, 
                                    const name& taker, 
                                    const name& order_no){
    if (_gstate.farm_lease_id <= 0) return;

    if (_gstate.farm_scales.count(make_asset.get_extended_symbol())){
        auto scale = _gstate.farm_scales.at(make_asset.get_extended_symbol());
        auto value = multiply_decimal64( make_asset.quantity.amount, 
                         get_precision(APLINK_SYMBOL), get_precision(make_asset.quantity.symbol));
        value = value * scale / get_precision(APLINK_SYMBOL);

        asset apples = asset(0, APLINK_SYMBOL);
        aplink::farm::available_apples(APLINK_FARM, _gstate.farm_lease_id, apples);
        if(apples.amount >= value && value > 0)
            ALLOT(  APLINK_FARM, _gstate.farm_lease_id, 
                    maker, asset(value, APLINK_SYMBOL), 
                    "fixswap allot: " + order_no.to_string() );
    }

    if ( _gstate.farm_scales.count(take_asset.get_extended_symbol())){
        auto scale = _gstate.farm_scales.at(take_asset.get_extended_symbol());
        auto value = multiply_decimal64( take_asset.quantity.amount, 
                         get_precision(APLINK_SYMBOL), get_precision(take_asset.quantity.symbol));
        value = value * scale / get_precision(APLINK_SYMBOL);

        asset apples = asset(0, APLINK_SYMBOL);
        aplink::farm::available_apples(APLINK_FARM, _gstate.farm_lease_id, apples);
        if(apples.amount >= value && value > 0)
            ALLOT(  APLINK_FARM, _gstate.farm_lease_id, 
                    taker, asset(value, APLINK_SYMBOL), 
                    "fixswap allot: " + order_no.to_string() );
    }
}

void fixswap::cancel(const name& maker, const name& order_no){
    require_auth(maker);
    
    CHECKC( _gstate.status == swap_status_t::initialized, err::PAUSED, "contract is maintaining" )

    auto swap_order = swap_t(order_no);
    CHECKC( _db.get(swap_order), err::RECORD_NOT_FOUND, "cannot found swap order" )

    CHECKC( swap_order.maker == maker, err::NO_AUTH, "no auth to cancel order" )
    CHECKC( swap_order.status == swap_status_t::matching, err::STATUS_MISMATCH, "order status mismatched" )

    TRANSFER(swap_order.make_asset.contract, 
        swap_order.maker, 
        swap_order.make_asset.quantity, 
        "Cancel swap order: " + order_no.to_string());

    _db.del(swap_order);
    notification(order_no, swap_status_t::cancel);
}


// void fixswap::clear(const vector<name>& order_nos){
//     require_auth(_gstate.admin);

//     CHECKC(order_nos.size()>0, err::PARAM_ERROR, "empty ids");

//     for (auto& order_no : order_nos) {
//         auto order = swap_t(order_no);
//         CHECKC( _db.get( order ), err::RECORD_NOT_FOUND, "order not found: " + order_no.to_string() )
//         CHECKC( order.status == swap_status_t::matched || order.status == swap_status_t::cancel,
//             err::STATUS_MISMATCH, "can only clear order in matched or cancel");
//         _db.del( order );
// 	}
// }

void fixswap::notification(const name& order_no, const name& status){
    require_auth(get_self());
}