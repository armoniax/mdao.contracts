#include "algoex.token.factory/algoex.token.factory.hpp"
#include <thirdparty/utils.hpp>
#include <amax.ntoken/did.ntoken_db.hpp>
#include <mdao.gov/mdao.gov.db.hpp>
#include <algoex.token/algoex.token.hpp>
#include <mdao.info/mdao.info.db.hpp>
#include <mdao.stake/mdao.stake.db.hpp>

using namespace std;

namespace algoextokenfactory {
// amcli push action tokenfactory1 createtoken '["amax.dao","ad",100,"测试","1000000.0000 ABQP","aa"]' -p ad
//memo: fullname|asset|metadata
//const name& creator, const string& fullname,const asset& maximum_supply, const string& metadata
ACTION tokenfactory::createtoken(const name& from, const name& to,
                                 const asset& quantity, const string& memo)
{
    if (from == _self || to != _self) return;

    auto conf = _conf();
    CHECKC( conf.status != conf_status::PENDING, factory_err::NOT_AVAILABLE, "under maintenance" );
    CHECKC( quantity >= conf.token_create_fee, factory_err::PARAM_ERROR, "incorrect handling fee")

    auto parts = split( memo, "|" );
    CHECKC( parts.size() == 3, factory_err::INVALID_FORMAT, "expected format: 'fullname | asset | metadata" );
  
    string_view fullname = string_view(parts[0]);
    CHECKC( fullname.size() <= 24, factory_err::INVALID_FORMAT, "fullname length is more than 24 bytes");

    string_view token_asset = string_view(parts[1]);
    CHECKC( token_asset.size() <= 132, factory_err::INVALID_FORMAT, "asset length is more than 132 bytes");
    asset maximum_supply = asset_from_string(token_asset);

    string_view metadata = string_view(parts[2]);
    CHECKC( metadata.size() <= 100, factory_err::INVALID_FORMAT, "metadata length is more than 100 bytes");

    CHECKC( maximum_supply.amount > 0, factory_err::NOT_POSITIVE, "not positive quantity:" + maximum_supply.to_string() )
    symbol_code supply_code = maximum_supply.symbol.code();
    CHECKC( supply_code.length() > 3, factory_err::NO_AUTH, "cannot create limited token" )
    CHECKC( !conf.black_symbols.count(supply_code) ,factory_err::NOT_ALLOW, "token not allowed to create" );

    stats statstable( MDAO_TOKEN, supply_code.raw() );
    CHECKC( statstable.find(supply_code.raw()) == statstable.end(), factory_err::CODE_REPEAT, "token already exist")

    auto claimer_acnts = amax::account_t::idx_t( did::DID_NTOKEN, from.value );
    bool is_auth = false;
    for( auto claimer_acnts_iter = claimer_acnts.begin(); claimer_acnts_iter!=claimer_acnts.end(); claimer_acnts_iter++ ){
        if(claimer_acnts_iter->balance.amount > 0){
            is_auth = true;
            break;
        } 
    }
    CHECKC( is_auth, factory_err::DID_NOT_AUTH, "did is not authenticated" );

    // user_stake_t::idx_t user_token_stake(MDAO_STAKE, MDAO_STAKE.value);
    // auto user_token_stake_index = user_token_stake.get_index<"unionid"_n>();
    // auto user_token_stake_iter = user_token_stake_index.find(mdao::get_unionid(from, code));
    // int64_t value = user_token_stake_iter->tokens_stake.at(extended_symbol{AMAX_SYMBOL, AMAX_TOKEN});
    // CHECKC( value >= 200000000000, factory_err::AMAX_NOT_ENOUGH, "amax pledged is insufficient" );

    XTOKEN_CREATE_TOKEN(MDAO_TOKEN, from, maximum_supply, fullname, metadata)
}

ACTION tokenfactory::issuetoken(const name& owner, const name& code, const name& to, 
                            const asset& quantity, const string& memo)
{
    auto conf = _conf();

    symbol_code supply_code = quantity.symbol.code();
    stats statstable( MDAO_TOKEN, supply_code.raw() );
    auto existing = statstable.find(supply_code.raw());
    CHECKC( existing != statstable.end(), factory_err::TOKEN_NOT_EXIST, "token not exist")
    const auto &st = *existing;
    check( has_auth(conf.managers[manager_type::FACTORY]) && (st.issuer == to), "insufficient permissions");
        
    XTOKEN_ISSUE(MDAO_TOKEN, to, quantity, memo)
}

const tokenfactory::conf_t& tokenfactory::_conf() {
    if (!_conf_ptr) {
        _conf_tbl_ptr = make_unique<conf_table_t>(MDAO_CONF, MDAO_CONF.value);
        check(_conf_tbl_ptr->exists(), "conf table not existed in contract" );
        _conf_ptr = make_unique<conf_t>(_conf_tbl_ptr->get());
    }
    return *_conf_ptr;
}

} 
