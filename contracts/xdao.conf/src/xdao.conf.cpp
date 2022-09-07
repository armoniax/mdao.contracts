#include <xdao.conf/xdao.conf.hpp>

ACTION xdaoconf::init( const name& feetaker, const app_info& appinfo, const asset& daoupgfee )
{
    require_auth( _self );
    check( is_account(feetaker), "feetaker not exits" );
    check(daoupgfee.symbol == AMAX_SYMBOL, "quantity symbol mismatch with daoupgfee symbol");
    _gstate.appinfo = appinfo;
    _gstate.fee_taker = feetaker;
    _gstate.dao_upg_fee = daoupgfee;
}

ACTION xdaoconf::daoconf( const name& feetaker,const app_info& appinfo, const name& status, const asset& daoupgfee )
{
    require_auth( _self );
    check(  conf_status::INITIAL == status  || conf_status::RUNNING == status || conf_status::MAINTAIN == status, "status illegal" );
    check(daoupgfee.amount > 0, "daoupgfee must positive quantity");
    check(daoupgfee.symbol == AMAX_SYMBOL, "quantity symbol mismatch with daoupgfee symbol");

    _gstate.appinfo = appinfo;
    _gstate.status = status;
    _gstate.dao_upg_fee = daoupgfee;
    _gstate.fee_taker = feetaker;
}

ACTION xdaoconf::seatconf( const uint16_t& amctokenmax, const uint16_t& evmtokenmax, uint16_t& dappmax )
{
    require_auth( _self );
    _gstate.amc_token_seats_max = amctokenmax;
    _gstate.evm_token_seats_max = evmtokenmax;
    _gstate.dapp_seats_max = dappmax;
}

ACTION xdaoconf::managerconf( const map<name, name>& managers )
{
    require_auth( _self );
    check( managers.size() != 0, "managers size cannot be zero" );
    for (const auto& item : managers) {
        check(  manager::INFO == item.second  || manager::STRATEGY == item.second || manager::WALLET == item.second || manager::TOKEN == item.second, "manager illegal" );
        _gstate.managers[item.first] = item.second;
    }
}

ACTION xdaoconf::setlimitcode( const symbol_code& symbolcode )
{
    require_auth( _self );
    _gstate.limited_symbols.insert(symbolcode);
}