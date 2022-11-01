#include "algoex.token.medium/algoex.token.medium.hpp"
#include <thirdparty/utils.hpp>
#include <amax.ntoken/did.ntoken_db.hpp>
#include <mdao.gov/mdao.gov.db.hpp>
#include <algoex.token/algoex.token.hpp>
#include <mdao.info/mdao.info.db.hpp>
#include <mdao.stake/mdao.stake.db.hpp>

using namespace std;

namespace mdaotokenmidium {
// amcli push action tokenmedium1 createtoken '["amax.dao","ad",100,"测试","1000000.0000 ABQP","aa"]' -p ad
ACTION tokenmedium::createtoken(const name& code, const name& creator, const uint16_t& transfer_ratio, 
                             const string& fullname, const asset& maximum_supply, const string& metadata)
{
    // CHECKC( false, medium_err::NOT_AVAILABLE, "under maintenance" );

    auto conf = _conf();
    CHECKC( conf.status != conf_status::PENDING, medium_err::NOT_AVAILABLE, "under maintenance" );

    CHECKC( fullname.size() <= 20, medium_err::SIZE_TOO_MUCH, "fullname has more than 20 bytes")
    CHECKC( maximum_supply.amount > 0, medium_err::NOT_POSITIVE, "not positive quantity:" + maximum_supply.to_string() )
    symbol_code supply_code = maximum_supply.symbol.code();
    CHECKC( supply_code.length() > 3, medium_err::NO_AUTH, "cannot create limited token" )
    CHECKC( !conf.black_symbols.count(supply_code) ,medium_err::NOT_ALLOW, "token not allowed to create" );

    stats statstable( MDAO_TOKEN, supply_code.raw() );
    CHECKC( statstable.find(supply_code.raw()) == statstable.end(), medium_err::CODE_REPEAT, "token already exist")

     _check_permission(code, creator, conf);

    auto claimer_acnts = amax::account_t::idx_t( did::DID_NTOKEN, creator.value );
    bool is_auth = false;
    for( auto claimer_acnts_iter = claimer_acnts.begin(); claimer_acnts_iter!=claimer_acnts.end(); claimer_acnts_iter++ ){
        if(claimer_acnts_iter->balance.amount > 0){
            is_auth = true;
            break;
        } 
    }
    CHECKC( is_auth, medium_err::DID_NOT_AUTH, "did is not authenticated" );

    user_stake_t::idx_t user_token_stake(MDAO_STAKE, MDAO_STAKE.value);
    auto user_token_stake_index = user_token_stake.get_index<"unionid"_n>();
    auto user_token_stake_iter = user_token_stake_index.find(mdao::get_unionid(creator, code));
    int64_t value = user_token_stake_iter->tokens_stake.at(extended_symbol{AMAX_SYMBOL, AMAX_TOKEN});
    CHECKC( value >= 200000000000, medium_err::AMAX_NOT_ENOUGH, "amax pledged is insufficient" );

    XTOKEN_CREATE_TOKEN(MDAO_TOKEN, creator, maximum_supply, transfer_ratio, fullname, code, metadata)
}

ACTION tokenmedium::issuetoken(const name& owner, const name& code, const name& to, 
                            const asset& quantity, const string& memo)
{
    auto conf = _conf();

    symbol_code supply_code = quantity.symbol.code();
    stats statstable( MDAO_TOKEN, supply_code.raw() );
    CHECKC( statstable.find(supply_code.raw()) != statstable.end(), medium_err::TOKEN_NOT_EXIST, "token not exist")

     _check_permission(code, owner, conf);
        
    XTOKEN_ISSUE(MDAO_TOKEN, to, quantity, memo)

}

void tokenmedium::_check_permission( const name& code, const name& creator,  const conf_t& conf ) {
    governance_t::idx_t governance_tbl(MDAO_GOV, MDAO_GOV.value);
    const auto governance = governance_tbl.find(code.value);
    CHECKC( (governance == governance_tbl.end() && has_auth(creator)) || (governance != governance_tbl.end() && has_auth(conf.managers.at(manager_type::PROPOSAL))), 
                medium_err::PERMISSION_DENIED, "permission denied" );
    
    dao_info_t::idx_t info_tbl(MDAO_INFO, MDAO_INFO.value);
    const auto info = info_tbl.find(code.value);
    CHECKC( info != info_tbl.end() ,medium_err::RECORD_NOT_FOUND, "record not found");
    CHECKC( (has_auth(conf.managers.at(manager_type::PROPOSAL)) ) || (has_auth(creator) && info->creator == creator), medium_err::PERMISSION_DENIED, "only the creator can operate" );
}

const tokenmedium::conf_t& tokenmedium::_conf() {
    if (!_conf_ptr) {
        _conf_tbl_ptr = make_unique<conf_table_t>(MDAO_CONF, MDAO_CONF.value);
        check(_conf_tbl_ptr->exists(), "conf table not existed in contract" );
        _conf_ptr = make_unique<conf_t>(_conf_tbl_ptr->get());
    }
    return *_conf_ptr;
}

} 
