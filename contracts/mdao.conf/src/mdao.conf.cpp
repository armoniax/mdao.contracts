#include <mdao.conf/mdao.conf.hpp>

ACTION mdaoconf::init( const name& feetaker, const app_info& appinfo, const asset& daoupgfee, const name& admin )
{
    require_auth( _self );
    check( is_account(feetaker), "feetaker not exits" );
    check(daoupgfee.symbol == AMAX_SYMBOL, "quantity symbol mismatch with daoupgfee symbol");
    _gstate.appinfo = appinfo;
    _gstate.fee_taker = feetaker;
    _gstate.dao_upgrade_fee = daoupgfee;
    _gstate.dao_admin = admin;

}

ACTION mdaoconf::daoconf( const name& feetaker,const app_info& appinfo, const name& status, const asset& daoupgfee )
{
    require_auth( _self );
    check(  conf_status::INITIAL == status  || conf_status::RUNNING == status || conf_status::MAINTAIN == status, "status illegal" );
    check(daoupgfee.amount > 0, "daoupgfee must positive quantity");
    check(daoupgfee.symbol == AMAX_SYMBOL, "quantity symbol mismatch with daoupgfee symbol");

    _gstate.appinfo = appinfo;
    _gstate.status = status;
    _gstate.dao_upgrade_fee = daoupgfee;
    _gstate.fee_taker = feetaker;
}

ACTION mdaoconf::seatconf( const uint16_t& amctokenmax, const uint16_t& evmtokenmax, uint16_t& dappmax )
{
    require_auth( _self );
    _gstate.token_seats_max = amctokenmax;
    _gstate.dapp_seats_max = dappmax;
}

ACTION mdaoconf::managerconf( const name& managetype, const name& manager )
{
    require_auth( _self );
    check( manager_type::INFO == managetype || manager_type::STRATEGY == managetype || manager_type::WALLET == managetype || manager_type::TOKEN == managetype, "manager illegal" );
    _gstate.managers[managetype] = manager;
}

ACTION mdaoconf::setlimitcode( const symbol_code& symbolcode )
{
    require_auth( _self );
    _gstate.limited_symbols.insert(symbolcode);
}

ACTION mdaoconf::reset()
{
    require_auth( _self );
}