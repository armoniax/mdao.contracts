#include "mdao.algoex.hpp"
#include <cmath>
#include <eosio/permission.hpp>
#include "thirdparty/utils.hpp"
#include <mdao.token/mdao.token.hpp>

using namespace mdao;
using namespace std;
using namespace eosio;


typedef double real_type;

void algoex::init(const name &admin){
    require_auth(get_self());

    CHECKC(_gstate.exchg_status == market_status::created, err::HAS_INITIALIZE, "exchange has initialized")

    _gstate.exchg_status = market_status::initialized;

    _gstate.admins[admin_type::admin] = admin;
    _gstate.admins[admin_type::tokenarc] = MDAO_BANK;
    _gstate.admins[admin_type::feetaker] = admin;

    // string top200[] = {"USDT","DOGE","WBTC","SHIB","AVAX","LINK","MATIC","NEAR","ALGO","ATOM","MANA","HBAR","THETA","TUSD","EGLD","SAND","IOTA","AAVE","WAVES","DASH","CAKE","SAFE","NEXO","CELO","KAVA","INCH","QTUM","IOST","IOTX","STORJ","ANKR","COMP","GUSD","HIVE","SUSHI","KEEP","POWR","ARDR","CELR","DENT","DYDX","STEEM","POLYX","NEST","TRAC","REEF","STPT","ALPHA","BAND","PERP","POND","AERGO","TOMO","BADGER","LOOM","ARPA","SERO","MONA","LINA","CTXC","DATA","IRIS","FIRO","YFII","AKRO","WNXM","NULS","QASH","FRONT","TIME","WICC"};
    // string arc200[] = {"AMAX","MUSDT","MBTC","METH","MBSC","MSOL","MEOS","MTRX","MDOT"};

    _gstate.limited_symbols.insert(SYS_SYMBOL.code());

    _gstate.quote_symbols.insert(extended_symbol(SYS_SYMBOL, SYS_BANK));
    _gstate.quote_symbols.insert(extended_symbol(SYMBOL("MUSDT", 6), MIRROR_BANK));
    _gstate.quote_symbols.insert(extended_symbol(SYMBOL("METH", 8), MIRROR_BANK));
    _gstate.quote_symbols.insert(extended_symbol(SYMBOL("MBTC", 8), MIRROR_BANK));

    _gstate.token_crt_fee = asset(asset_from_string("1.00000000 AMAX"));
    _gstate.exchg_fee_ratio = 100;
    _global.set(_gstate, get_self());
}

void algoex::setlimitsym(const set<string>& sym_codes){
    require_auth(_gstate.admins[admin_type::admin]);

    CHECKC(_gstate.exchg_status != market_status::created, err::UN_INITIALIZE, "please init exchange first")

    for (set<string>::iterator iter = sym_codes.begin(); iter != sym_codes.end(); ++iter){
        symbol_code code = symbol_code(iter->data());
        if(_gstate.limited_symbols.count(code) > 0) continue;

        _gstate.limited_symbols.insert(code);
    }
    _global.set(_gstate, get_self());
}

void algoex::setadmin(const name& admin_type,const name &admin){
    CHECKC(_gstate.exchg_status != market_status::created, err::UN_INITIALIZE, "please init exchange first")

    require_auth(_gstate.admins[admin_type::admin]);
    _gstate.admins[admin_type] = admin;
    _global.set(_gstate, get_self());
}

void algoex::setstatus(const name& status_type){
    require_auth(_gstate.admins[admin_type::admin]);

    CHECKC(_gstate.exchg_status != market_status::created, err::UN_INITIALIZE, "please init first")
    _gstate.exchg_status = status_type;
    _global.set(_gstate, get_self());
}

void algoex::updateappinf(const name& creator,
                const symbol_code& base_code,
                const AppInfo_t& app_info
                ){
    auto market = market_t(base_code);
    CHECKC(_db.get(market), err::RECORD_EXISTING, "market not exsits")
    CHECKC(creator == market.creator, err::NO_AUTH, "no auth to modify market")

    market.app_info = app_info;
    _db.set(market, get_self());
}

void algoex::setlauncher(const name& creator,
                    const symbol_code& base_code,
                    const name& launcher,
                    const string& recv_memo
                ){
    require_auth(creator);

    auto market = market_t(base_code);
    CHECKC(_db.get(market), err::RECORD_EXISTING, "market not exsits")
    CHECKC(creator == market.creator, err::NO_AUTH, "no auth to modify market")
    CHECKC(market.status == market_status::created, err::NO_AUTH, "can only modify in created status")

    market.launcher.owner = launcher;
    market.launcher.transmemo = recv_memo;

    _db.set(market, get_self());
}

void algoex::settaxtaker(const name& creator,
                    const symbol_code& base_code,
                    const name& taxker,
                    const string& recv_memo
                ){
    require_auth(creator);

    auto market = market_t(base_code);
    CHECKC(_db.get(market), err::RECORD_EXISTING, "market not exsits")
    CHECKC(creator == market.creator, err::NO_AUTH, "no auth to modify market")
    CHECKC(market.status == market_status::created, err::NO_AUTH, "can only modify in created status")

    market.taxtaker.owner = taxker;
    market.taxtaker.transmemo = recv_memo;

    _db.set(market, get_self());
}

void algoex::setmktstatus(const name& creator,
                const symbol_code& base_code,
                const name& status
                ){
    require_auth(creator);

    auto market = market_t(base_code);
    CHECKC(_db.get(market), err::RECORD_EXISTING, "market not exsits")
    CHECKC(creator == market.creator, err::NO_AUTH, "no auth to modify market")
    if(market.status == market_status::trading){
        CHECKC(market.status == market_status::suspended, err::PARAM_ERROR, "can only resume market in suspended status")
    }
    if(market.status == market_status::suspended){
        CHECKC(market.status == market_status::trading, err::PARAM_ERROR, "can only pause market in trading status")
    }
    else {
        CHECKC(false, err::PARAM_ERROR, "can only set market in suspended/trading status")
    }
    market.status = status;
    _db.set(market, get_self());
}

void algoex::ontransfer(const name &from, const name &to, const asset &quantity, const string &memo){
    if(from == get_self()) return;
    CHECKC(to == get_self() && to != from, err::ACCOUNT_INVALID, "invalid account")

    CHECKC(_gstate.exchg_status == market_status::trading, err::MAINTAINING, "exchange is in maintaining")

    vector<string_view> memo_params = split(memo, ":");

    CHECKC(memo_params.size()>0, err::PARAM_ERROR, "invalid memo");
    name action_type(memo_params.at(0));
    switch (action_type.value)
    {
        case transfer_type::create.value:{
            CHECKC(quantity == _gstate.token_crt_fee, err::ASSET_MISMATCH, "require creating fee: " + _gstate.token_crt_fee.to_string())
            _create_market(from, memo_params);
        }
        break;
        case transfer_type::launch.value: {
            _launch_market(from, quantity, memo_params);
        }
        break;
        case transfer_type::bid.value: {
            CHECKC(memo_params.size() == 2, err::PARAM_ERROR, "invalid params")
            symbol_code base_code = symbol_code(memo_params.at(1));
            _bid(from, quantity, base_code);
        }
        break;
        case transfer_type::ask.value: {
            CHECKC(memo_params.size() == 2, err::PARAM_ERROR, "invalid params")
            symbol_code base_code = symbol_code(memo_params.at(1));
            _ask(from, quantity, base_code);
        }
        break;
        default:
            CHECKC(false, err::PARAM_ERROR, "unsupport type")
            break;
    }
}

void algoex::onissue(const name &to, const asset &quantity, const string &memo){
    CHECKC(get_first_receiver() == _gstate.admins.at(admin_type::tokenarc), err::ACCOUNT_INVALID, "require issue from " + _gstate.admins.at(admin_type::tokenarc).to_string())
    CHECKC(_gstate.exchg_status == market_status::trading, err::MAINTAINING, "exchange is in maintaining")

    CHECKC(to == get_self(), err::ACCOUNT_INVALID, "issued to must be self")
    market_t market(quantity.symbol.code());
    CHECKC(_db.get(market), err::RECORD_NOT_FOUND ,"cannot found market")
    CHECKC(market.status == market_status::created, err::TIME_EXPIRED, "must issued in created status")
    CHECKC(market.base_balance.contract == get_first_receiver(),
        err::ACCOUNT_INVALID, "require issued from " + market.base_balance.contract.to_string())
    CHECKC(quantity.amount > 0, err::NOT_POSITIVE, "require positve amount")

    market.base_balance.quantity += quantity;
    market.status = market_status::initialized;
    _db.set(market, get_self());
}

void algoex::_create_market(const name& creator,
                            const vector<string_view>& memo_params
                        ){
    CHECKC(get_first_receiver() == SYS_BANK, err::ACCOUNT_INVALID, "require fee from " + SYS_BANK.to_string())
    CHECKC(memo_params.size() >= 8, err::PARAM_ERROR, "invalid params")

    name arc = _gstate.admins.at(admin_type::tokenarc);

    asset base_supply = asset_from_string(memo_params.at(1));
    uint16_t in_tax = to_uint16(memo_params.at(2), "in_tax value error:");
    uint16_t out_tax = to_uint16(memo_params.at(3), "out_tax value error:");
    uint16_t parent_rwd_rate = to_uint16(memo_params.at(4), "parent_rwd_rate value error:");
    uint16_t grand_rwd_rate = to_uint16(memo_params.at(5), "grand_rwd_rate value error:");
    uint16_t token_fee_ratio = to_uint16(memo_params.at(6), "token_fee_ratio value error:");
    uint16_t token_gas_ratio = to_uint16(memo_params.at(7), "token_gas_ratio value error:");

    CHECKC(base_supply.amount>0, err::NOT_POSITIVE, "not positive quantity:" + base_supply.to_string())

    symbol_code base_code = base_supply.symbol.code();

    CHECKC(base_code.length() > 3, err::NO_AUTH, "cannot create limited token")
    CHECKC(_gstate.limited_symbols.count(base_code) == 0, err::NO_AUTH, "cannot create limited token")
    CHECKC(base_code != BRIDGE_SYMBOL.code(), err::NO_AUTH, "BRIDGE name is limited")

    auto market = market_t(base_code);
    CHECKC(!_db.get(market), err::RECORD_EXISTING, "symbol is existed")

    CHECKC(in_tax >= 0 && in_tax <= 0.2 * RATIO_BOOST, err::OVERSIZED, "in tax should in range 0-20%")
    CHECKC(out_tax >= 0 && out_tax <= 0.2 * RATIO_BOOST, err::OVERSIZED, "out tax should in range 0-20%")
    CHECKC(token_fee_ratio >= 0 && token_fee_ratio <= 0.01 * RATIO_BOOST, err::OVERSIZED, "in tax should in range 0-20%")
    CHECKC(token_gas_ratio >= 0 && token_gas_ratio <= 0.01 * RATIO_BOOST, err::OVERSIZED, "out tax should in range 0-20%")
    CHECKC(parent_rwd_rate >= 0 && parent_rwd_rate <= 0.2 * RATIO_BOOST, err::OVERSIZED, "parent_rwd_rate tax should less than 20%")
    CHECKC(grand_rwd_rate >= 0 && grand_rwd_rate <= 0.2 * RATIO_BOOST, err::OVERSIZED, "grand_rwd_rate tax should less than 20%")
    CHECKC((parent_rwd_rate + grand_rwd_rate <= in_tax) && (parent_rwd_rate + grand_rwd_rate <= out_tax),
            err::OVERSIZED, "reward ratio should less than in_tax and out_tax")

    market.base_code = base_code;
    market.creator = creator;
    market.status = market_status::created;
    market.in_tax = in_tax;
    market.out_tax = out_tax;
    market.parent_rwd_rate = parent_rwd_rate;
    market.grand_rwd_rate = grand_rwd_rate;
    market.base_supply = base_supply;
    market.base_balance = extended_asset(asset(0, base_supply.symbol), arc);
    Depository_t depository;
    depository.owner = creator;
    depository.transmemo = base_code.to_string();
    market.launcher = depository;
    market.taxtaker = depository;
    market.created_at = current_time_point();

    _db.set(market, get_self());

    XTOKEN_TRANSFER(SYS_BANK, _gstate.admins.at(admin_type::feetaker), _gstate.token_crt_fee, base_code.to_string() + " market creation fee")
    XTOKEN_ISSUE(arc, get_self(), base_supply, "")
}

void algoex::_launch_market(const name& launcher,
                            const asset& quantity,
                            const vector<string_view>& memo_params){
    CHECKC(memo_params.size() >= 5, err::PARAM_ERROR, "invalid params")

    name arc = get_first_receiver();
    auto market = market_t(symbol_code(memo_params.at(1)));
    name algo_type = name(memo_params.at(2));
    asset quote_supply = asset_from_string(memo_params.at(3));

    CHECKC(_db.get(market), err::RECORD_NOT_FOUND ,"cannot found market")
    CHECKC(market.status == market_status::initialized, err::HAS_INITIALIZE, "cannot launch market in status: " + market.status.to_string())
    CHECKC(market.launcher.owner == launcher, err::NO_AUTH, "no auth to launch market")

    CHECKC(quote_supply.amount>0, err::NOT_POSITIVE, "not positive quantity:" + quote_supply.to_string())
    CHECKC(_gstate.quote_symbols.count(extended_symbol(quote_supply.symbol, arc)), err::SYMBOL_MISMATCH, "unvalid quote asset: " + quote_supply.to_string())

    CHECKC((algo_type == algo_type_t::bancor || algo_type == algo_type_t::polycurve), err::PARAM_ERROR, "unsopport algo type")

    market.algo_type = algo_type;
    market.quote_balance = extended_asset(asset(0, quote_supply.symbol), arc);
    market.quote_supply = quote_supply;

    switch (algo_type.value)
    {
    case algo_type_t::polycurve.value: {
        asset lauch_price = asset_from_string(memo_params.at(4));
        _launch_polycurve_market(market, launcher, quantity, lauch_price);
        }
        break;
    case algo_type_t::bancor.value:
        check(false, "bancor algo is devloping");
        break;
    default:
        check(false, "unsupport algo type");
    }
}

void algoex::_launch_polycurve_market(
                            market_t market,
                            const name& launcher,
                            const asset& quantity,
                            const asset& lauch_price){

    int64_t bvalue = multiply_decimal64( lauch_price.amount, SLOPE_BOOST, get_precision(lauch_price) );
    int64_t max_price = divide_decimal64(market.quote_supply.amount, market.base_balance.quantity.amount,  get_precision(market.base_balance.quantity) );
    int64_t delta_price = max_price - lauch_price.amount;
    int64_t aslope_tmp = multiply_decimal64(delta_price, SLOPE_BOOST, get_precision(lauch_price));
    int64_t aslope = divide_decimal64(aslope_tmp, market.base_balance.quantity.amount,  get_precision(market.base_balance.quantity) );
    CHECKC(aslope >= 0, err::NOT_POSITIVE, "calculated slope cannot be a negtive value")

    market.algo_params[algo_parma_type::bvalue] = bvalue;
    market.algo_params[algo_parma_type::aslope] = aslope;
    market.status = market_status::trading;

    asset lauch_out = market.convert(quantity, market.base_balance.quantity.symbol);
    CHECKC(lauch_out.amount != 0, err::PARAM_ERROR, "lauch failed: " + lauch_out.to_string())
    XTOKEN_TRANSFER(market.base_balance.contract, launcher, lauch_out, "launch out with " + quantity.to_string())

    _db.set(market, get_self());
}


void algoex::_bid(const name& account, const asset& quantity, const symbol_code& base_code){
    auto market = market_t(base_code);

    CHECKC(_db.get(market), err::RECORD_NOT_FOUND ,"cannot found market")
    CHECKC(market.status == market_status::trading, err::MAINTAINING, "market is in maintaining")
    CHECKC(quantity.symbol == market.quote_balance.quantity.symbol, err::SYMBOL_MISMATCH, "symbol mismatch")
    name arc = get_first_receiver();
    CHECKC( arc == market.quote_balance.contract, err::SYMBOL_MISMATCH, "invalid asset from " + arc.to_string())

    asset fee = asset((int64_t)multiply_decimal64(quantity.amount, _gstate.exchg_fee_ratio, RATIO_BOOST), quantity.symbol);
    asset tax = asset((int64_t)multiply_decimal64(quantity.amount, market.in_tax, RATIO_BOOST), quantity.symbol);
    asset actual_trade = quantity - fee - tax;

    if(actual_trade.amount > 0){
        asset exchg_quantity = market.convert(actual_trade, market.base_balance.quantity.symbol);
        asset avg = asset((int64_t)divide_decimal64(quantity.amount, exchg_quantity.amount, power10(exchg_quantity.symbol.precision())), actual_trade.symbol);

        CHECKC(exchg_quantity.amount > 0, err::NOT_POSITIVE, actual_trade.to_string() + " is too small to exchange: "+exchg_quantity.to_string())
        CHECKC(market.base_balance.quantity.amount >= 0, err::OVERSIZED, "market balance not enough")
        XTOKEN_TRANSFER(market.base_balance.contract, account, exchg_quantity, "price: "+avg.to_string())
    }

    if(fee.amount > 0)
        XTOKEN_TRANSFER(arc, _gstate.admins.at(admin_type::feetaker), fee, "exchange fee")

    if(tax.amount > 0)
        _allot_tax(account, market, tax, arc);

    _db.set(market, get_self());
}


void algoex::_ask(const name& account, const asset& quantity, const symbol_code& base_code){
    auto market = market_t(base_code);

    CHECKC(_db.get(market), err::RECORD_NOT_FOUND ,"cannot found market")
    CHECKC(market.status == market_status::trading, err::MAINTAINING, "market is in maintaining")
    CHECKC(quantity.symbol == market.base_balance.quantity.symbol, err::SYMBOL_MISMATCH, "symbol mismatch")
    name arc = get_first_receiver();
    CHECKC( arc == market.base_balance.contract, err::SYMBOL_MISMATCH, "invalid asset from " + arc.to_string())

    asset exchg_quantity = market.convert(quantity, market.quote_supply.symbol);
    asset avg = asset((int64_t)divide_decimal64(exchg_quantity.amount, quantity.amount,  power10(quantity.symbol.precision())), exchg_quantity.symbol);

    CHECKC(exchg_quantity.amount > 0, err::NOT_POSITIVE, "quantity is too small to exchange")
    CHECKC(market.quote_balance.quantity.amount >= 0, err::OVERSIZED, "market quote not enough")

    asset fee = asset((int64_t)multiply_decimal64(exchg_quantity.amount, _gstate.exchg_fee_ratio, RATIO_BOOST), exchg_quantity.symbol);
    asset tax = asset((int64_t)multiply_decimal64(exchg_quantity.amount, market.out_tax, RATIO_BOOST), exchg_quantity.symbol);

    asset actual_trade = exchg_quantity - fee - tax;

    if(actual_trade.amount > 0)
        XTOKEN_TRANSFER(market.quote_balance.contract, account, exchg_quantity, "price: "+avg.to_string())

    if(fee.amount > 0)
        XTOKEN_TRANSFER(market.quote_balance.contract, _gstate.admins.at(admin_type::feetaker), fee, "exchange fee")

    if(tax.amount > 0)
        _allot_tax(account, market, tax, market.quote_balance.contract);

    _db.set(market, get_self());
}


void algoex::_allot_tax(const name& account, const market_t& market, const asset& tax, const name& bank_con){
    asset parent_tax = asset((int64_t)multiply_decimal64(tax.amount, market.parent_rwd_rate, RATIO_BOOST), tax.symbol);
    asset grand_tax = asset((int64_t)multiply_decimal64(tax.amount, market.grand_rwd_rate, RATIO_BOOST), tax.symbol);
    asset actual_tax = tax;

    name parent = get_account_creator(account);
    if(parent_tax.amount > 0 && parent != SYS_ACCT){
        actual_tax -= parent_tax;
        XTOKEN_TRANSFER(bank_con, parent, parent_tax, "algoex reward from "+account.to_string())
    }
    if(grand_tax.amount > 0 && parent != SYS_ACCT){
        name grand = get_account_creator(parent);
        if(grand != SYS_ACCT){
            actual_tax -= grand_tax;
            XTOKEN_TRANSFER(bank_con, grand, grand_tax, "algoex reward from "+account.to_string())
        }
    }
    if(actual_tax.amount > 0)
        XTOKEN_TRANSFER(bank_con, market.taxtaker.owner, actual_tax, market.taxtaker.transmemo)
}