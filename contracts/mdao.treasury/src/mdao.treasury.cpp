#include <mdao.treasury/mdao.treasury.hpp>
#include <mdao.info/mdao.info.db.hpp>
#include <mdao.gov/mdao.gov.db.hpp>
#include <mdao.info/mdao.info.db.hpp>
#include <mdao.stg/mdao.stg.hpp>
#include <thirdparty/utils.hpp>
#include <set>
#define TOKEN_TRANSFER(bank, to, quantity, memo) \
{ action(permission_level{get_self(), "active"_n }, bank, "transfer"_n, std::make_tuple( _self, to, quantity, memo )).send(); }


void mdaotreasury::ontrantoken( name from, name to, asset quantity, string memo )
{
    if (from == _self || to != _self) return;

    auto conf = _conf();
    CHECKC( conf.status != conf_status::PENDING, treasury_err::NOT_AVAILABLE, "under maintenance" );

	CHECKC( quantity.amount > 0, treasury_err::NOT_POSITIVE, "quantity must be positive" )

    name dao_code = name(memo);
    
    dao_info_t::idx_t info_tbl(MDAO_INFO, MDAO_INFO.value);
    const auto info = info_tbl.find(dao_code.value);
    CHECKC( info != info_tbl.end(), treasury_err::RECORD_NOT_FOUND, "dao not found" );
    
    treasury_balance_t treasury_balance(dao_code);
    bool is_exist = _db.get(treasury_balance);
    if(is_exist){
        extended_symbol ex_symbol(quantity.symbol, name(get_first_receiver()));
        treasury_balance.stake_assets[ex_symbol] = treasury_balance.stake_assets[ex_symbol] + quantity.amount;
        _db.set(treasury_balance, _self);
    }else{
        extended_symbol ex_symbol(quantity.symbol, name(get_first_receiver()));
        treasury_balance.stake_assets[ex_symbol] = quantity.amount;
        _db.set(treasury_balance, _self);
    }
}

void mdaotreasury::tokentranout( name dao_code, name to, extended_asset quantity, string memo )
{
    if (to != _self) return;

    auto conf = _conf();
    require_auth( conf.managers[manager_type::PROPOSAL] );
    CHECKC( conf.status != conf_status::PENDING, treasury_err::NOT_AVAILABLE, "under maintenance" );

    CHECKC( quantity.quantity.amount > 0, treasury_err::NOT_POSITIVE, "quantity must be positive" )

    treasury_balance_t treasury_balance(dao_code);
    CHECKC( _db.get(treasury_balance), treasury_err::RECORD_NOT_FOUND, "dao not found" )

    uint64_t amount = treasury_balance.stake_assets[quantity.get_extended_symbol()];
    CHECKC( amount > quantity.quantity.amount, treasury_err::INSUFFICIENT_BALANCE, "not sufficient funds " )

    //划转资产，扣减国库余额
    TOKEN_TRANSFER(quantity.contract, to, quantity.quantity, memo)

    treasury_balance.stake_assets[quantity.get_extended_symbol()] = treasury_balance.stake_assets[quantity.get_extended_symbol()] - quantity.quantity.amount;
    _db.set(treasury_balance, _self);
    
}

const mdaotreasury::conf_t& mdaotreasury::_conf() {
    if (!_conf_ptr) {
        _conf_tbl_ptr = make_unique<conf_table_t>(MDAO_CONF, MDAO_CONF.value);
        CHECKC(_conf_tbl_ptr->exists(), treasury_err::SYSTEM_ERROR, "conf table not existed in contract" );
        _conf_ptr = make_unique<conf_t>(_conf_tbl_ptr->get());
    }
    return *_conf_ptr;
}
