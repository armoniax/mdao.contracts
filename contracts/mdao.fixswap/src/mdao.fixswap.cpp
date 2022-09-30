#include "mdao.fixswap.hpp"
#include "aplink.farm/aplink.farm.hpp"

static constexpr eosio::name active_permission{"active"_n};

#define ALLOT(bank, land_id, customer, quantity, memo) \
    {	aplink::farm::allot_action act{ bank, { {_self, active_perm} } };\
			act.send( land_id, customer, quantity , memo );}

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

void fixswap::init(const name& fee_collector, const uint32_t& fee_ratio){
    require_auth(get_self());

    CHECKC( _gstate.status == swap_status_t::created, err::HAS_INITIALIZE, "cannot initialize again" )
    CHECKC( is_account(fee_collector), err::ACCOUNT_INVALID, "cannot found fee_collector" )
    CHECKC( fee_ratio >= 0 && fee_ratio <= 1000, err::ACCOUNT_INVALID, "fee_ratio must be in range 0~10% (0~1000)" )

    _gstate.status = swap_status_t::initialized;
    _gstate.fee_collector = fee_collector;
    _gstate.take_fee_ratio = fee_ratio;
    _gstate.make_fee_ratio = fee_ratio;
    _gstate.supported_contracts.insert("amax.token"_n);
    _gstate.supported_contracts.insert("amax.mtoken"_n);
    _gstate.supported_contracts.insert("mdao.token"_n);
    _global.set( _gstate, get_self());
}

void fixswap::setfee(const name& fee_collector, const uint32_t& take_fee_ratio, const uint32_t& make_fee_ratio){
    require_auth(_gstate.fee_collector);

    CHECKC( is_account(fee_collector), err::ACCOUNT_INVALID, "cannot found fee_collector" )
    CHECKC( take_fee_ratio >= 0 && take_fee_ratio <= 1000, err::NOT_POSITIVE, "take_fee_ratio must be in range 0~10% (0~1000)" )
    CHECKC( make_fee_ratio >= 0 && make_fee_ratio <= 1000, err::NOT_POSITIVE, "make_fee_ratio must be in range 0~10% (0~1000)" )

    _gstate.fee_collector = fee_collector;
    _gstate.take_fee_ratio = take_fee_ratio;
    _gstate.make_fee_ratio = make_fee_ratio;
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
    CHECKC( _gstate.supported_contracts.count(get_first_receiver()), err::SYMBOL_MISMATCH, "unsupport token contract" )

    vector<string_view> params = split(memo, ":");
    CHECKC( params.size() > 0, err::PARAM_ERROR, "unsupport memo" )

    if(params.size() == 6 && params[0] == "make"){
        auto swap_order = swap_t(name(params[1]));
        CHECKC( !_db.get(swap_order), err::RECORD_EXISTING, "repeat swap order" )
        swap_order.id = _gstate.swap_id++;
        swap_order.maker = from;
        swap_order.created_at = current_time_point();
        swap_order.make_asset = extended_asset(quantity, get_first_receiver());

        if(params[2].length() > 0){
            name taker(params[2]);
            CHECKC( is_account(taker), err::ACCOUNT_INVALID, "cannot found taker account" )
            swap_order.taker = taker;
        }

        asset take_quant = asset_from_string(params[3]);
        CHECKC( take_quant.amount > 0, err::ACCOUNT_INVALID, "take quanity must be positive" )

        name take_contract = name(params[4]);
        CHECKC( is_account(take_contract), err::ACCOUNT_INVALID, "cannot found take quantity contract" )

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
        // extended_asset take_asset = std::get<extended_asset>(swap_order.take_asset);
        // extended_asset make_asset = std::get<extended_asset>(swap_order.make_asset);

        CHECKC( quantity == swap_order.take_asset.quantity, err::SYMBOL_MISMATCH, "swap quantity mismatch" )
        CHECKC( get_first_receiver() == swap_order.take_asset.contract, err::SYMBOL_MISMATCH, "quantity contract mismatch" )

        asset make_fee = swap_order.make_asset.quantity * _gstate.make_fee_ratio / percent_boost;
        TRANSFER(swap_order.make_asset.contract, 
            from, 
            swap_order.make_asset.quantity - make_fee, 
            "Swap with " + swap_order.make_asset.quantity.to_string());

        if(make_fee.amount > 0){
            TRANSFER(swap_order.make_asset.contract, 
                _gstate.fee_collector, 
                make_fee, 
                "Swap take fee of " + swap_orderno.to_string());

            if (_gstate.farm_lease_id > 0 && _gstate.farm_scales.count(swap_order.make_asset.get_extended_symbol())){
                auto scale = _gstate.farm_scales.at(swap_order.make_asset.get_extended_symbol());
                auto value = multiply_decimal64( make_fee.amount, get_precision(APLINK_SYMBOL), get_precision(make_fee.symbol));
                value = value * scale / percent_boost;
                asset apples = asset(0, APLINK_SYMBOL);
                aplink::farm::available_apples(APLINK_FARM, _gstate.farm_lease_id, apples);
                if(apples.amount >= value && value > 0)
                    ALLOT(  APLINK_FARM, _gstate.farm_lease_id, swap_order.maker, asset(value, APLINK_SYMBOL), 
                            "fixswap allot: "+swap_orderno.to_string() );
            }
        }



        asset take_fee = swap_order.take_asset.quantity * _gstate.take_fee_ratio / percent_boost;

        TRANSFER(swap_order.take_asset.contract, 
            swap_order.maker, 
            swap_order.take_asset.quantity - take_fee, 
            "Swap with " + swap_order.take_asset.quantity.to_string());
        
        if(take_fee.amount > 0){
            TRANSFER(swap_order.take_asset.contract, 
                _gstate.fee_collector, 
                take_fee, 
                "Swap take fee of " + swap_orderno.to_string());

            if (_gstate.farm_lease_id > 0 && _gstate.farm_scales.count(swap_order.take_asset.get_extended_symbol())){
                auto scale = _gstate.farm_scales.at(swap_order.take_asset.get_extended_symbol());
                auto value = multiply_decimal64( take_fee.amount, get_precision(APLINK_SYMBOL), get_precision(take_fee.symbol));
                value = value * scale / percent_boost;
                asset apples = asset(0, APLINK_SYMBOL);
                aplink::farm::available_apples(APLINK_FARM, _gstate.farm_lease_id, apples);
                if(apples.amount >= value && value > 0)
                    ALLOT(  APLINK_FARM, _gstate.farm_lease_id, swap_order.maker, asset(value, APLINK_SYMBOL), 
                            "fixswap allot: "+swap_orderno.to_string() );
            }
        }

        _db.del(swap_order);
    }
}

// void fixswap::onnftsawp(name from, name to, vector< nasset >& assets, string memo){
//     if(from == get_self() || to != get_self()) return;
//     CHECKC( _gstate.status == swap_status_t::initialized, err::PAUSED, "contract is maintaining" )
    
//     CHECKC( assets.size() != 1, err::PARAM_ERROR, "assets size must be equal to 1" )

//     nasset quantity = assets[0];
//     CHECKC( quantity.amount>0, err::NOT_POSITIVE, "swap quanity must be positive" )

//     vector<string_view> params = split(memo, ":");
//     CHECKC( params.size() > 0, err::PARAM_ERROR, "unsupport memo" )

//     if(params.size() == 4 && params[0] == "nftmake"){
//         auto swap_order = swap_t(_gstate.swap_id++);
//         swap_order.maker = from;
//         swap_order.make_asset = quantity;

//         if(params[1].length() > 0){
//             name taker(params[1]);
//             CHECKC( is_account(taker), err::ACCOUNT_INVALID, "cannot found taker account" )
//             swap_order.taker = taker;
//         }
        
//         //pid|id|amount
//         vector<string_view> nftinfo = split(params[2], "|");
//         uint64_t pid = to_uint64(nftinfo[0], "pid error");
//         uint64_t id = to_uint64(nftinfo[1], "id error");
//         uint64_t take_amount = to_uint64(nftinfo[2], "amount error");

//         CHECKC( take_amount > 0, err::ACCOUNT_INVALID, "take quanity must be positive" )
//         swap_order.take_asset = nasset(pid, id, take_amount);
//         // asset take_quant = asset_from_string(params[2]);

//         // name take_contract = name(params[3]);
//         // CHECKC( is_account(take_contract), err::ACCOUNT_INVALID, "cannot found take quantity contract" )

//         // swap_order.take_asset = extended_asset(take_quant, take_contract);
//         if(params[3].length() > 0){
//             swap_order.code = params[4];
//         }
//         swap_order.expired_at = time_point_sec(current_time_point()) + default_expired_secs;

//         _db.set(swap_order, get_self());
//         _global.set( _gstate, get_self());
//     }
//     else if(params.size() == 3 && params[0] == "take"){
//         auto swap_id = to_uint64(params[1], "swap id error:");
//         auto swap_order = swap_t(swap_id);
//         CHECKC( _db.get(swap_order), err::RECORD_NOT_FOUND, "cannot found swap order" )
//         CHECKC( time_point_sec(current_time_point()) < swap_order.expired_at , err::TIME_EXPIRED, "swap order expired")

//         if(swap_order.taker != name(0)) {
//             CHECKC( swap_order.taker == from, err::NO_AUTH, "no auth to swap order" )
//         }
//         //
//         else if(swap_order.code.size() != 0){
//             string data = string(params[2]);
//             checksum256 digest = sha256(&data[0], data.size());
//             auto bytes = digest.extract_as_byte_array();
//             string hexstr = to_hex(bytes.data(), bytes.size());
//             CHECKC( hexstr == swap_order.code, err::NO_AUTH, "auth failed: code mismatch: " + data + " | " + hexstr)
//         }
//         nasset take_asset = std::get<nasset>(swap_order.take_asset);
//         nasset make_asset = std::get<nasset>(swap_order.make_asset);

//         CHECKC( quantity.amount == take_asset.amount, err::SYMBOL_MISMATCH, "swap quantity mismatch" )
//         // CHECKC( get_first_receiver() == take_asset.contract, err::SYMBOL_MISMATCH, "quantity contract mismatch" )

//         vector<nasset> make_quants = { make_asset };
//         TRANSFER_N( NFT_BANK, from, make_quants, "swap nft: " + to_string(swap_order.id) );

//         vector<nasset> take_quants = { take_asset };
//         TRANSFER_N( NFT_BANK, swap_order.maker, take_quants, "swap nft: " + to_string(swap_order.id) );


//         // TRANSFER(make_asset.contract, 
//         //     from, 
//         //     swap_order.make_asset.quantity, 
//         //     "Swap with " + swap_order.make_asset.quantity.to_string());

//         // asset fee = swap_order.take_asset.quantity * _gstate.fee_ratio / percent_boost;

//         // TRANSFER(swap_order.take_asset.contract, 
//         //     swap_order.maker, 
//         //     swap_order.take_asset.quantity - fee, 
//         //     "Swap with " + swap_order.take_asset.quantity.to_string());
        
//         // if(fee.amount > 0){
//         //     TRANSFER(swap_order.take_asset.contract, 
//         //         _gstate.fee_collector, 
//         //         fee, 
//         //         "Swap fee of " + to_string(swap_id));

//         //     if (_gstate.farm_lease_id > 0 && _gstate.farm_scales.count(swap_order.take_asset.get_extended_symbol())){
//         //         auto scale = _gstate.farm_scales.at(swap_order.take_asset.get_extended_symbol());
//         //         auto value = multiply_decimal64( fee.amount, get_precision(APLINK_SYMBOL), get_precision(fee.symbol));
//         //         value = value * scale / percent_boost;
//         //         asset apples = asset(0, APLINK_SYMBOL);
//         //         aplink::farm::available_apples(APLINK_FARM, _gstate.farm_lease_id, apples);
//         //         if(apples.amount >= value && value > 0)
//         //             ALLOT(  APLINK_FARM, _gstate.farm_lease_id, swap_order.maker, asset(value, APLINK_SYMBOL), 
//         //                     "fixswap allot: "+to_string(swap_id) );
//         //     }
//         // }

//         _db.del(swap_order);
//     }

// }

void fixswap::cancel(const name& maker, const name& orderno){
    require_auth(maker);
    
    CHECKC( _gstate.status == swap_status_t::initialized, err::PAUSED, "contract is maintaining" )

    auto swap_order = swap_t(orderno);
    CHECKC( _db.get(swap_order), err::RECORD_NOT_FOUND, "cannot found swap order" )

    CHECKC( swap_order.maker == maker, err::NO_AUTH, "no auth to cancel order" )
    
    // extended_asset make_asset = std::get<extended_asset>(swap_order.make_asset);
    TRANSFER(swap_order.make_asset.contract, 
        swap_order.maker, 
        swap_order.make_asset.quantity, 
        "Cancel swap order: " + orderno.to_string());

    _db.del(swap_order);
}