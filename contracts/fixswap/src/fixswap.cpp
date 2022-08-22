#include <thirdparty/utils.hpp>
#include <fixswap.hpp>
#include "eosio.token/eosio.token.hpp"
#include "aplink.farm/aplink.farm.hpp"
#include "thirdparty/utils.hpp"

static constexpr eosio::name active_permission{"active"_n};

#define ALLOT(bank, land_id, customer, quantity, memo) \
    {	aplink::farm::allot_action act{ bank, { {_self, active_perm} } };\
			act.send( land_id, customer, quantity , memo );}

using namespace xdao;
using namespace std;
using namespace eosio;

inline int64_t get_precision(const symbol &s) {
    int64_t digit = s.precision();
    CHECK(digit >= 0 && digit <= 18, "precision digit " + std::to_string(digit) + " should be in range[0,18]");
    return calc_precision(digit);
}

inline int64_t get_precision(const asset &a) {
    return get_precision(a.symbol);
}

void fixswap::init(const name& fee_collector, const uint32_t& fee_ratio){
    require_auth(get_self());

    CHECKC( _gstate.status == swap_status_t::created, err::HAS_INITIALIZE, "cannot initialize again" )
    CHECKC( is_account(fee_collector), err::ACCOUNT_INVALID, "cannot found fee_collector" )
    CHECKC( fee_ratio >= 0 && fee_ratio <= 1000, err::ACCOUNT_INVALID, "fee_ratio must be in range 0~10% (0~1000)" )

    _gstate.status = swap_status_t::initialized;
    _gstate.fee_collector = fee_collector;
    _gstate.fee_ratio = fee_ratio;
    _global.set( _gstate, get_self());
}

void fixswap::setfee(const name& fee_collector, const uint32_t& fee_ratio){
    require_auth(_gstate.fee_collector);

    CHECKC( is_account(fee_collector), err::ACCOUNT_INVALID, "cannot found fee_collector" )
    CHECKC( fee_ratio >= 0 && fee_ratio <= 1000, err::NOT_POSITIVE, "fee_ratio must be in range 0~10% (0~1000)" )

    _gstate.fee_collector = fee_collector;
    _gstate.fee_ratio = fee_ratio;
    _global.set( _gstate, get_self());
}

void fixswap::setfarm(const uint64_t& farm_lease_id, const map<extended_symbol, uint32_t>& farm_scales){
    require_auth(_gstate.fee_collector);

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

    vector<string_view> params = split(memo, ":");
    CHECKC( params.size() > 0, err::PARAM_ERROR, "unsupport memo" )

    if(params.size() == 5 && params[0] == "make"){
        auto swap_order = swap_t(_gstate.swap_id++);
        swap_order.maker = from;
        swap_order.make_asset = extended_asset(quantity, get_first_receiver());

        if(params[1].length() > 0){
            name taker(params[1]);
            CHECKC( is_account(taker), err::ACCOUNT_INVALID, "cannot found taker account" )
            swap_order.taker = taker;
        }

        asset take_quant = asset_from_string(params[2]);
        CHECKC( take_quant.amount > 0, err::ACCOUNT_INVALID, "take quanity must be positive" )

        name take_contract = name(params[3]);
        CHECKC( is_account(take_contract), err::ACCOUNT_INVALID, "cannot found take quantity contract" )

        swap_order.take_asset = extended_asset(take_quant, take_contract);
        if(params[4].length() > 0){
            swap_order.code = params[4];
        }
        swap_order.expired_at = current_time_point() + default_expired_secs;

        _db.set(swap_order, get_self());
        _global.set( _gstate, get_self());
    }
    else if(params.size() == 3 && params[0] == "take"){
        auto swap_id = to_uint64(params[1], "swap id error:");
        auto swap_order = swap_t(swap_id);
        CHECKC( _db.get(swap_order), err::RECORD_NOT_FOUND, "cannot found swap order" )
        CHECKC( current_time_point < swap_order.expired_at , err::TIME_EXPIRED, "swap order expired")

        if(swap_order.taker != name(0)) {
            CHECKC( swap_order.taker == from, err::NO_AUTH, "no auth to swap order" )
        }
        else if(swap_order.code.size() != 0){
            string data = string(params[2]);
            checksum256 digest = sha256(&data[0], data.size());
            auto bytes = digest.extract_as_byte_array();
            string hexstr = to_hex(bytes.data(), bytes.size());
            CHECKC( hexstr == swap_order.code, err::NO_AUTH, "auth failed: code mismatch: " + data + " | " + hexstr)
        }
        CHECKC( quantity == swap_order.take_asset.quantity, err::SYMBOL_MISMATCH, "swap quantity mismatch" )
        CHECKC( get_first_receiver() == swap_order.take_asset.contract, err::SYMBOL_MISMATCH, "quantity contract mismatch" )

        TRANSFER(swap_order.make_asset.contract, 
            from, 
            swap_order.make_asset.quantity, 
            "Swap with " + swap_order.make_asset.quantity.to_string());

        asset fee = swap_order.take_asset.quantity * _gstate.fee_ratio / percent_boost;

        TRANSFER(swap_order.take_asset.contract, 
            swap_order.maker, 
            swap_order.take_asset.quantity - fee, 
            "Swap with " + swap_order.take_asset.quantity.to_string());
        
        if(fee.amount > 0){
            TRANSFER(swap_order.take_asset.contract, 
                _gstate.fee_collector, 
                fee, 
                "Swap fee of " + to_string(swap_id));

            if (_gstate.farm_lease_id > 0 && _gstate.farm_scales.count(swap_order.take_asset.get_extended_symbol())){
                auto scale = _gstate.farm_scales.at(swap_order.take_asset.get_extended_symbol());
                auto value = multiply_decimal64( fee.amount, get_precision(APLINK_SYMBOL), get_precision(fee.symbol));
                value = value * scale / percent_boost;
                asset apples = asset(0, APLINK_SYMBOL);
                aplink::farm::available_apples(APLINK_FARM, _gstate.farm_lease_id, apples);
                if(apples.amount >= value && value > 0)
                    ALLOT(  APLINK_FARM, _gstate.farm_lease_id, swap_order.maker, asset(value, APLINK_SYMBOL), 
                            "fixswap allot: "+to_string(swap_id) );
            }
        }

        _db.del(swap_order);
    }
}

void fixswap::cancel(const name& maker, const uint64_t& swap_id){
    require_auth(maker);
    
    CHECKC( _gstate.status == swap_status_t::initialized, err::PAUSED, "contract is maintaining" )

    auto swap_order = swap_t(swap_id);
    CHECKC( _db.get(swap_order), err::RECORD_NOT_FOUND, "cannot found swap order" )

    CHECKC( swap_order.maker == maker, err::NO_AUTH, "no auth to cancel order" )

    TRANSFER(swap_order.make_asset.contract, 
        swap_order.maker, 
        swap_order.make_asset.quantity, 
        "Cancel swap order: " + to_string(swap_id));

    _db.del(swap_order);
}