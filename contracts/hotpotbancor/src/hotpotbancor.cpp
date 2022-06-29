#include "hotpotbancor.hpp"
#include "thirdparty/utils.hpp"
#include "eosio.token/eosio.token.hpp"
#include <cmath>
#include <eosio/permission.hpp>

using namespace hotpot;
using namespace std;
using namespace eosio;

void bancor::init(const name &admin){
    require_auth(get_self());

    CHECKC(_gstate.exchg_status == market_status::created, err::HAS_INITIALIZE, "exchange has initialized")

    _gstate.exchg_status = market_status::initialized;

    _gstate.admins[admin_type::admin] = admin;
    _gstate.admins[admin_type::tokenarc] = HOTPOT_BANK;
    _gstate.admins[admin_type::feetaker] = admin;

    string_view top200[] = {"USDT","DOGE","WBTC","SHIB","AVAX","LINK","MATIC","NEAR","ALGO","ATOM","MANA","HBAR","THETA","TUSD","EGLD","SAND","IOTA","AAVE","WAVES","DASH","CAKE","SAFE","NEXO","CELO","KAVA","1INCH","QTUM","IOST","IOTX","STORJ","ANKR","COMP","GUSD","HIVE","SUSHI","KEEP","POWR","ARDR","CELR","DENT","DYDX","STEEM","POLYX","NEST","TRAC","REEF","STPT","ALPHA","BAND","PERP","POND","AERGO","TOMO","BADGER","LOOM","ARPA","SERO","MONA","LINA","CTXC","DATA","IRIS","FIRO","YFII","AKRO","WNXM","NULS","QASH","FRONT","TIME","WICC"};
    string_view arc200[] = {"AMAX","MUSDT","MBTC","METH","MBSC","MSOL","MEOS","MTRX","MDOT"};
    for (size_t i = 0; i < sizeof(top200); i++)
        _gstate.limited_symbols.insert(symbol_code(top200[i]));
    for (size_t i = 0; i < sizeof(arc200); i++)
        _gstate.limited_symbols.insert(symbol_code(arc200[i]));

    _gstate.quote_symbols.insert(extended_symbol(SYS_SYMBOL, SYS_BANK));
    _gstate.quote_symbols.insert(extended_symbol(SYMBOL("MUSDT", 6), MIRROR_BANK));
    _gstate.quote_symbols.insert(extended_symbol(SYMBOL("METH", 8), MIRROR_BANK));
    _gstate.quote_symbols.insert(extended_symbol(SYMBOL("MBTC", 8), MIRROR_BANK));

    _gstate.token_crt_fee = asset(asset_from_string("1.00000000 AMAX"));
    _gstate.exchg_fee_ratio = 100;
    _global.set(_gstate, get_self());
}

void bancor::setadmin(const name& admin_type,const name &admin){
    CHECKC(_gstate.exchg_status != market_status::created, err::UN_INITIALIZE, "please init exchange first")

    require_auth(_gstate.admins[admin_type::admin]);
    _gstate.admins[admin_type] = admin;
    _global.set(_gstate, get_self());
}

void bancor::setstatus(const name& status_type){
    require_auth(_gstate.admins[admin_type::admin]);
    
    _gstate.exchg_status = status_type;
    _global.set(_gstate, get_self());
}

void bancor::ontransfer(name from, name to, asset quantity, string memo){
    if(from == get_self()) return;
    CHECKC(to!=get_self() && to != from, err::ACCOUNT_INVALID, "invalid account")

    CHECKC(_gstate.exchg_status == market_status::trading, err::MAINTAINING, "exchange is in maintaining")

    vector<string_view> memo_params = split(memo, ":");
    
    CHECKC(memo_params.size()>0, err::PARAM_ERROR, "invalid memo");
    name action_type(memo_params.at(0));
    switch (action_type)
    {
        case transfer_type::create:{
            CHECKC(quantity == _gstate.token_crt_fee, err::ASSET_MISMATCH, "require creating fee: " + _gstate.token_crt_fee.to_string())
            CHECKC(get_first_receiver() == SYS_BANK, err::ACCOUNT_INVALID, "require fee from " + SYS_BANK.to_string())
            CHECKC(memo_params.size() == 10, err::PARAM_ERROR, "invalid params")
            uint16_t cw_value = to_uint16(memo_params.at(1), "CW value error:");
            asset base_supply = asset_from_string(memo_params.at(2));
            asset quote_supply = asset_from_string(memo_params.at(3));
            name quote_contract(memo_params.at(4));
            uint16_t in_tax = to_uint16(memo_params.at(5), "in_tax value error:");
            uint16_t out_tax = to_uint16(memo_params.at(6), "out_tax value error:");
            uint16_t parent_rwd_rate = to_uint16(memo_params.at(7), "parent_rwd_rate value error:");
            uint16_t grand_rwd_rate = to_uint16(memo_params.at(8), "grand_rwd_rate value error:");

            _create_market(from, cw_value, 
                extended_asset(base_supply, _gstate.admins.at(admin_type::tokenarc)), 
                extended_asset(quote_supply, quote_contract),
                in_tax, out_tax, parent_rwd_rate, grand_rwd_rate);
            
            TRANSFER(SYS_BANK, _gstate.admins.at(admin_type::feetaker), quantity, "creation fee")
        }
        break;
        case transfer_type::lauch: {
            CHECKC(memo_params.size() == 2, err::PARAM_ERROR, "invalid params")
            symbol_code base_code = symbol_code(memo_params.at(1));
            _launch_market(from, quantity, base_code);
        }
        break;
        case transfer_type::bid: {
            CHECKC(memo_params.size() == 2, err::PARAM_ERROR, "invalid params")
            symbol_code base_code = symbol_code(memo_params.at(1));
            _bid(from, quantity, base_code);
        }
        break;
        case transfer_type::ask: {
            CHECKC(memo_params.size() == 2, err::PARAM_ERROR, "invalid params")
            symbol_code base_code = symbol_code(memo_params.at(1));
            _ask(from, quantity, base_code);
        }
        break;
        default:
            break;
    }
}

void bancor::onissue(const name &to, const asset &quantity, const string &memo){
    CHECKC(_gstate.exchg_status == market_status::trading, err::MAINTAINING, "exchange is in maintaining")
    
    CHECKC(to == get_self(), err::ACCOUNT_INVALID, "issued to must be self")
    market_t market(quantity.symbol.code());
    CHECKC(_db.get(market), err::RECORD_NOT_FOUND ,"cannot found market")
    CHECKC(market.status != market_status::created, err::TIME_EXPIRED, "must issued in created status")
    CHECKC(market.base_balance.contract == get_first_receiver(), 
        err::ACCOUNT_INVALID, "require issued from " + market.base_balance.contract.to_string())
    CHECKC(quantity.amount > 0, err::NOT_POSITIVE, "require positve amount")

    market.base_balance.quantity += quantity;
    CHECKC(market.base_balance.quantity > market.base_supply, err::OVERSIZED, "issued balance shouble be less than supply")
    if(market.base_balance.quantity == market.base_supply)
        market.status = market_status::initialized;
    _db.set(market, get_self());
}

asset  bancor::_convert_to_exchange(market_t& market, extended_asset& reserve, const asset& payment ){
      const double S0 = market.bridge_supply.amount;
      const double R0 = reserve.quantity.amount;
      const double dR = payment.amount;
      const double F  = market.cw_value/RATIO_BOOST;

      double dS = S0 * ( std::pow(1. + dR / R0, F) - 1. );
      if ( dS < 0 ) dS = 0; // rounding errors
      reserve.quantity += payment;
      market.bridge_supply.amount   += int64_t(dS);
      return asset( int64_t(dS), market.bridge_supply.symbol );
}

asset bancor::_convert_from_exchange(market_t& market, extended_asset& reserve, const asset& tokens ){
      const double R0 = reserve.quantity.amount;
      const double S0 = market.bridge_supply.amount;
      const double dS = -tokens.amount; // dS < 0, tokens are subtracted from supply
      const double Fi = double(1) / (market.cw_value/RATIO_BOOST);

      double dR = R0 * ( std::pow(1. + dS / S0, Fi) - 1. ); // dR < 0 since dS < 0
      if ( dR > 0 ) dR = 0; // rounding errors
      reserve.quantity.amount -= int64_t(-dR);
      market.bridge_supply                 -= tokens;
      return asset( int64_t(-dR), reserve.get_extended_symbol().get_symbol());
}

asset bancor::_convert(market_t& market, const asset& from, const symbol& to)
{
    const auto& sell_symbol  = from.symbol;
    const auto& base_symbol  = market.base_supply.symbol;
    const auto& quote_symbol = market.quote_supply.symbol;

    check( sell_symbol != to, "cannot convert to the same symbol" );

    asset out( 0, to );
    if ( sell_symbol == base_symbol && to == quote_symbol ) {
        const asset tmp = _convert_to_exchange(market, market.base_balance, from );
        out = _convert_from_exchange(market, market.quote_balance, tmp );
    } else if ( sell_symbol == quote_symbol && to == base_symbol ) {
        const asset tmp = _convert_to_exchange(market, market.quote_balance, from );
        out = _convert_from_exchange(market, market.base_balance, tmp );
    } else {
        check( false, "invalid conversion" );
    }
    return out;
}

void bancor::_create_market(const name& creator, 
                        const uint16_t& cw_value,
                        const extended_asset& base_supply,
                        const extended_asset& quote_supply,
                        const uint16_t& in_tax,
                        const uint16_t& out_tax,
                        const uint16_t& parent_rwd_rate,
                        const uint16_t& grand_rwd_rate
                        ){
    CHECKC((cw_value > 0 && cw_value <= RATIO_BOOST), err::PARAM_ERROR, "cw should between 1-10000")
    CHECKC(base_supply.quantity.amount>0, err::NOT_POSITIVE, "not positive quantity:" + base_supply.quantity.to_string())
    CHECKC(quote_supply.quantity.amount>0, err::NOT_POSITIVE, "not positive quantity:" + quote_supply.quantity.to_string())
    CHECKC(_gstate.quote_symbols.count(quote_supply.get_extended_symbol()), err::SYMBOL_MISMATCH, "unvalid quote asset: " + quote_supply.quantity.to_string())

    symbol_code base_code = base_supply.quantity.symbol.code();

    CHECKC(base_code.length() > 3, err::NO_AUTH, "cannot create limited token")
    CHECKC(_gstate.limited_symbols.count(base_code) == 0, err::NO_AUTH, "cannot create limited token")
    CHECKC(base_code == BRIDGE_SYMBOL.code(), err::NO_AUTH, "BRIDGE name is limited")
    
    auto market = market_t(base_code);
    CHECKC(in_tax >= 0 && in_tax <= 0.2*RATIO_BOOST, err::OVERSIZED, "in tax should in range 0-20%")
    CHECKC(out_tax >= 0 && out_tax <= 0.2*RATIO_BOOST, err::OVERSIZED, "out tax should in range 0-20%")
    CHECKC(parent_rwd_rate >= 0 && parent_rwd_rate <= 0.2*RATIO_BOOST, err::OVERSIZED, "parent_rwd_rate tax should less than 20%")
    CHECKC(grand_rwd_rate >= 0 && grand_rwd_rate <= 0.2*RATIO_BOOST, err::OVERSIZED, "grand_rwd_rate tax should less than 20%")
    CHECKC((parent_rwd_rate + grand_rwd_rate <= in_tax) && (parent_rwd_rate + grand_rwd_rate <= out_tax),
            err::OVERSIZED, "reward ratio should less than in_tax and out_tax")

    CHECKC(!_db.get(market), err::RECORD_EXISTING, "symbol is existed")
    
    market.base_code = base_code;
    market.creator = creator;
    market.status = market_status::created;
    market.base_supply = base_supply.quantity;
    market.quote_supply = quote_supply.quantity;
    market.bridge_supply = asset(BRIDGE_AMOUNT, BRIDGE_SYMBOL);
    market.base_balance = extended_asset(asset(0, base_supply.get_extended_symbol().get_symbol()), base_supply.contract);
    market.quote_balance = extended_asset(asset(0, quote_supply.get_extended_symbol().get_symbol()), quote_supply.contract);
    market.in_tax = in_tax;
    market.out_tax = out_tax;
    market.parent_rwd_rate = parent_rwd_rate;
    market.grand_rwd_rate = grand_rwd_rate;
    Depository_t depository;
    depository.owner = creator;
    depository.transmemo = market.base_code.to_string();
    market.laucher = depository;
    market.taxtaker = depository;
    market.created_at = current_time_point();

    _db.set(market, get_self());
}

void bancor::_launch_market(const name& launcher, 
                            const asset& quantity,
                            const symbol_code& base_code){
    auto market = market_t(base_code);

    CHECKC(_db.get(market), err::RECORD_NOT_FOUND ,"cannot found market")
    CHECKC(market.status == market_status::created, err::HAS_INITIALIZE, "cannot lauch market in status: " + market.status.to_string())
    CHECKC(market.laucher.owner == launcher, err::NO_AUTH, "no auth to lauch market")
    CHECKC(quantity.symbol == market.quote_balance.quantity.symbol, err::SYMBOL_MISMATCH, "symbol mismatch")
    name arc = get_first_receiver();
    CHECKC( arc == market.quote_balance.contract, err::SYMBOL_MISMATCH, "invalid asset from " + arc.to_string())

    asset exchg_quantity = _convert(market, quantity, market.base_supply.symbol);
    CHECKC(exchg_quantity.amount > 0, err::NOT_POSITIVE, "quantity is too small to exchange")
    CHECKC(market.base_balance.quantity > exchg_quantity, err::OVERSIZED, "market base balance not enough")
        
    market.quote_balance.quantity += quantity;
    market.base_balance.quantity -= exchg_quantity;
    TRANSFER(market.base_balance.contract, launcher, exchg_quantity, "exchange with " + quantity.to_string())
    
    market.status = market_status::trading;
    _db.set(market, get_self());
}

void bancor::_bid(const name& account, const asset& quantity, const symbol_code& base_code){
    auto market = market_t(base_code);

    CHECKC(_db.get(market), err::RECORD_NOT_FOUND ,"cannot found market")
    CHECKC(market.status == market_status::trading, err::MAINTAINING, "market is in maintaining")
    CHECKC(quantity.symbol == market.quote_balance.quantity.symbol, err::SYMBOL_MISMATCH, "symbol mismatch")
    name arc = get_first_receiver();
    CHECKC( arc == market.quote_balance.contract, err::SYMBOL_MISMATCH, "invalid asset from " + arc.to_string())

    asset fee = quantity * _gstate.exchg_fee_ratio / RATIO_BOOST;
    asset tax = quantity * market.in_tax / RATIO_BOOST;
    asset actual_trade = quantity - fee - tax;

    if(actual_trade.amount > 0){
        asset exchg_quantity = _convert(market, actual_trade, market.base_supply.symbol);
        CHECKC(exchg_quantity.amount > 0, err::NOT_POSITIVE, "quantity is too small to exchange")
        CHECKC(market.base_balance.quantity > exchg_quantity, err::OVERSIZED, "market balance not enough")
        
        market.quote_balance.quantity += actual_trade;
        market.base_balance.quantity -= exchg_quantity;
        TRANSFER(market.base_balance.contract, account, exchg_quantity, "exchange with " + quantity.to_string())
    }

    if(fee.amount > 0)
        TRANSFER(arc, _gstate.admins.at(admin_type::feetaker), fee, "exchange fee")

    if(tax.amount > 0)
        _allot_tax(account, market, tax, arc);

    _db.set(market, get_self());
}


void bancor::_ask(const name& account, const asset& quantity, const symbol_code& base_code){
    auto market = market_t(base_code);

    CHECKC(_db.get(market), err::RECORD_NOT_FOUND ,"cannot found market")
    CHECKC(market.status == market_status::trading, err::MAINTAINING, "market is in maintaining")
    CHECKC(quantity.symbol == market.base_balance.quantity.symbol, err::SYMBOL_MISMATCH, "symbol mismatch")
    name arc = get_first_receiver();
    CHECKC( arc == market.base_balance.contract, err::SYMBOL_MISMATCH, "invalid asset from " + arc.to_string())

    asset exchg_quantity = _convert(market, quantity, market.quote_supply.symbol);
    CHECKC(exchg_quantity.amount > 0, err::NOT_POSITIVE, "quantity is too small to exchange")
    CHECKC(market.quote_balance.quantity > exchg_quantity, err::OVERSIZED, "market balance not enough")
    
    market.base_balance.quantity += quantity;
    market.quote_balance.quantity -= exchg_quantity;

    asset fee = exchg_quantity * _gstate.exchg_fee_ratio / RATIO_BOOST;
    asset tax = exchg_quantity * market.out_tax / RATIO_BOOST;
    asset actual_trade = exchg_quantity - fee - tax;

    if(actual_trade.amount > 0)
        TRANSFER(market.quote_balance.contract, account, exchg_quantity, "exchange with " + quantity.to_string())

    if(fee.amount > 0)
        TRANSFER(market.quote_balance.contract, _gstate.admins.at(admin_type::feetaker), fee, "exchange fee")

    if(tax.amount > 0)
        _allot_tax(account, market, tax, market.quote_balance.contract);

    _db.set(market, get_self());
}


void bancor::_allot_tax(const name& account, const market_t& market, const asset& tax, const name& bank_con){
    asset parent_tax = tax * market.parent_rwd_rate/RATIO_BOOST;
    asset grand_tax = tax * market.grand_rwd_rate/RATIO_BOOST;
    asset actual_tax = tax;
    name parent = get_account_creator(account);
    if(parent_tax.amount > 0 && parent != SYS_ACCT){
        actual_tax -= parent_tax;
        TRANSFER(bank_con, parent, parent_tax, "bancor reward from "+account.to_string())
    }
    if(grand_tax.amount > 0 && parent != SYS_ACCT){
        name grand = get_account_creator(parent);
        if(grand != SYS_ACCT){
            actual_tax -= grand_tax;
            TRANSFER(bank_con, grand, grand_tax, "bancor reward from "+account.to_string())
        }
    }
    if(actual_tax.amount > 0)
        TRANSFER(bank_con, market.taxtaker.owner, actual_tax, market.taxtaker.transmemo)
}