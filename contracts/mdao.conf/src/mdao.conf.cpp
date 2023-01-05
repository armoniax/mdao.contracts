#include <mdao.conf/mdao.conf.hpp>

ACTION mdaoconf::init( const name& fee_taker, const app_info& app_info, const asset& dao_upg_fee, const name& admin, const name& status )
{
    require_auth( _self );

    check( is_account(fee_taker), "feetaker not exits" );
    check( dao_upg_fee.symbol == AMAX_SYMBOL, "quantity symbol mismatch with daoupgfee symbol" );
    check( conf_status::INITIAL == status || conf_status::RUNNING == status || conf_status::PENDING == status, "status illegal" );
   
    _gstate.appinfo         = app_info;
    _gstate.fee_taker       = fee_taker;
    _gstate.upgrade_fee     = dao_upg_fee;
    _gstate.admin           = admin;
    _gstate.status          = status;

}

ACTION mdaoconf::setseat( uint16_t& dappmax )
{
    require_auth( _self );
    _gstate.dapp_seats_max = dappmax;
}

ACTION mdaoconf::setmanager( const name& manage_type, const name& manager )
{
    require_auth( _self );
    // check( manager_type::INFO == managetype || manager_type::STRATEGY == managetype 
    //         || manager_type::WALLET == managetype || manager_type::TOKEN == managetype, "manager illegal" );
    // check( manager_type::INFO == manage_type, "manager illegal" );
    _gstate.managers[manage_type] = manager;
}

ACTION mdaoconf::setsystem( const name& token_contract, const name& ntoken_contract, uint16_t stake_period_days )
{    
    require_auth( _self );
    if(token_contract.length() != 0)    _gstate.token_contracts.insert(token_contract);
    if(ntoken_contract.length() != 0)   _gstate.ntoken_contracts.insert(ntoken_contract);
    if(stake_period_days != 0)          _gstate.stake_period_days = stake_period_days;
}

ACTION mdaoconf::setmetaverse( const bool& enable_metaverse )
{    
    require_auth( _self );
    _gstate.enable_metaverse = enable_metaverse;
}

ACTION mdaoconf::settokenfee( const asset& quantity )
{    
    require_auth( _self );
    _gstate.token_create_fee = quantity;
}

ACTION mdaoconf::settag( const string& tag )
{    
    require_auth( _self );
    _gstate.available_tags.insert(tag);
}

ACTION mdaoconf::deltag( const string& tag )
{    
    require_auth( _self );
    _gstate.available_tags.erase(tag);
}