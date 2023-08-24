#include <mdao.conf/mdao.conf.hpp>
#include <thirdparty/utils.hpp>

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

ACTION mdaoconf::setmeeting( bool& meeting_switch )
{
    require_auth( _self );
    _gstate3.meeting_switch = meeting_switch;
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
    check( find_substr(tag, string(".")) != -1 && find_substr(tag, string(" ")) == -1, "expected format: 'code.tag'");

    auto parts = split( tag, "." );
    check( parts.size() == 2, "invalid format" );
    name tag_code = name(parts[0]);

    vector<string> _tags = _gstate2.available_tags.at(tag_code).tags;
    check( count(_tags.begin(), _tags.end(), tag) == 0, "tag already exists" );

    _tags.push_back(tag);
    _gstate2.available_tags[tag_code].tags = _tags;
}

ACTION mdaoconf::deltag( const string& tag )
{
    require_auth( _self );

    auto parts = split( tag, "." );
    check( parts.size() == 2, "invalid format" );
    name tag_code = name(parts[0]);

    vector<string> _tags = _gstate2.available_tags.at(tag_code).tags;
    bool is_exist = false;
    for (vector<string>::iterator iter = _tags.begin(); iter!=_tags.end(); iter++) {
        if ( *iter == tag ){
            _tags.erase(iter);
            is_exist = true;
            break;
        }
    }
    check( is_exist, "tag not found");
    _gstate2.available_tags[tag_code].tags = _tags;
}

ACTION mdaoconf::settokencrtr( const name& creator )
{
    require_auth( _self );
    _gstate2.token_creator_whitelist.insert(creator);
}

ACTION mdaoconf::deltokencrtr( const name& creator )
{
    require_auth( _self );
    _gstate2.token_creator_whitelist.erase(creator);
}

ACTION mdaoconf::setplanid( const uint64_t& planid )
{
    require_auth( _self );
    _gstate2.custody_plan_id = planid;
}

ACTION mdaoconf::setthreshold( const asset& threshold )
{
    require_auth( _self );
    _gstate2.crt_token_threshold = threshold;
}

ACTION mdaoconf::setblacksym( const symbol_code& sym )
{
    require_auth( _self );
    _gstate.black_symbols.insert(sym);
}
