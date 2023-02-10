#include "mdao.token.factory/mdao.token.factory.hpp"
#include <thirdparty/utils.hpp>
#include <amax.custody/custodydb.hpp>
#include <amax.ntoken/did.ntoken_db.hpp>
#include <mdao.token/mdao.token.hpp>
#include <mdao.info/mdao.info.db.hpp>
#include <mdao.stake/mdao.stake.db.hpp>

using namespace std;
using namespace wasm::db;

namespace mdaotokenfactory {


ACTION tokenfactory::ontransfer(const name& from, const name& to,
                                 const asset& quantity, const string& memo)
{
    if (from == _self || to != _self) return;

    auto conf = _conf();
    auto conf2 = _conf2();
    CHECKC( conf.status != conf_status::PENDING, factory_err::NOT_AVAILABLE, "under maintenance" );
    CHECKC( quantity >= conf.token_create_fee, err::PARAM_ERROR, "incorrect handling fee")

    memo_params memoparam = _memo_analysis(memo, conf);
    _did_auth_check(from);

    if( conf2.crt_token_threshold.amount > 0 && conf2.token_creator_whitelist.count(from) == 0  )
        _custody_check(from, conf2);

    XTOKEN_CREATE_TOKEN(MDAO_TOKEN, from, memoparam.maximum_supply, memoparam.fullname, memoparam.metadata)
}

ACTION tokenfactory::issuetoken(const name& owner, const name& to,
                            const asset& quantity, const string& memo)
{
    auto conf = _conf();
    check( has_auth(owner), "insufficient permissions");

    symbol_code supply_code = quantity.symbol.code();
    stats statstable( MDAO_TOKEN, supply_code.raw() );
    auto existing = statstable.find(supply_code.raw());
    CHECKC( existing != statstable.end(), factory_err::TOKEN_NOT_EXIST, "token not exist")
    const auto &st = *existing;
    check( st.issuer == to, "insufficient permissions");

    XTOKEN_ISSUE(MDAO_TOKEN, to, quantity, memo)
}

memo_params tokenfactory::_memo_analysis(const string& memo, const conf_t& conf )
{
    auto parts = split( memo, ":" );
    CHECKC( parts.size() == 3, err::MEMO_FORMAT_ERROR, "expected format: 'fullname : asset : metadata" );

    string_view fullname = parts[0];
    CHECKC( fullname.size() <= 24, err::MEMO_FORMAT_ERROR, "fullname length is more than 24 bytes");

    string_view token_asset = parts[1];
    CHECKC( token_asset.size() <= 132, err::MEMO_FORMAT_ERROR, "asset length is more than 132 bytes");
    asset maximum_supply = asset_from_string(token_asset);

    string_view metadata = parts[2];
    CHECKC( metadata.size() <= 100, err::MEMO_FORMAT_ERROR, "metadata length is more than 100 bytes");

    CHECKC( maximum_supply.amount > 0, err::NOT_POSITIVE, "not positive quantity:" + maximum_supply.to_string() )
    symbol_code supply_code = maximum_supply.symbol.code();
    CHECKC( supply_code.length() > 3, err::NO_AUTH, "cannot create limited token" )
    CHECKC( !conf.black_symbols.count(supply_code) ,factory_err::NOT_ALLOW, "token not allowed to create" );

    stats statstable( MDAO_TOKEN, supply_code.raw() );
    CHECKC( statstable.find(supply_code.raw()) == statstable.end(), factory_err::CODE_REPEAT, "token already exist")

    memo_params memoparam;
    memoparam.maximum_supply = maximum_supply;
    memoparam.metadata       = metadata;
    memoparam.fullname       = fullname;
    return memoparam;
}

void tokenfactory::_did_auth_check( const name& from )
{
    auto did_acnts = amax::account_t::idx_t( did::DID_NTOKEN, from.value );
    bool is_auth = false;
    for( auto did_acnts_iter = did_acnts.begin(); did_acnts_iter!=did_acnts.end(); did_acnts_iter++ ){
        if(did_acnts_iter->balance.amount > 0){
            is_auth = true;
            break;
        }
    }
    CHECKC( is_auth, factory_err::DID_NOT_AUTH, "did is not authenticated" );
}

void tokenfactory::_custody_check( const name& from, const conf_t2& conf)
{
    issue_t::tbl_t custody_issue(AMAX_CUSTODY, AMAX_CUSTODY.value);
    auto custody_issue_index = custody_issue.get_index<"planreceiver"_n>();
    uint128_t plan_receiver_id = (uint128_t)conf.custody_plan_id << 64 | (uint128_t)from.value;
    auto custody_issue_itr = custody_issue_index.find(plan_receiver_id);
    CHECKC( custody_issue_itr->locked.symbol == conf.crt_token_threshold.symbol, factory_err::STATE_MISMATCH, "lock plan asset symbol not match" );

    int64_t amount = 0;
    for (uint32_t i = 0; custody_issue_itr != custody_issue_index.end(); custody_issue_itr++, i++) {
        amount += custody_issue_itr->locked.amount;
    }
    CHECKC( amount >= conf.crt_token_threshold.amount, factory_err::AMAX_NOT_ENOUGH, "amax pledged is insufficient" );

}

const tokenfactory::conf_t& tokenfactory::_conf() {
    if (!_conf_ptr) {
        _conf_tbl_ptr = make_unique<conf_table_t>(MDAO_CONF, MDAO_CONF.value);
        CHECKC(_conf_tbl_ptr->exists(), err::SYSTEM_ERROR, "conf table not existed in contract" );
        _conf_ptr = make_unique<conf_t>(_conf_tbl_ptr->get());
    }
    return *_conf_ptr;
}

const tokenfactory::conf_t2& tokenfactory::_conf2() {
    if (!_conf_ptr2) {
        _conf_tbl_ptr2 = make_unique<conf_table_t2>(MDAO_CONF, MDAO_CONF.value);
        CHECKC(_conf_tbl_ptr2->exists(), err::SYSTEM_ERROR, "conf table2 not existed in contract" );
        _conf_ptr2 = make_unique<conf_t2>(_conf_tbl_ptr2->get());
    }
    return *_conf_ptr2;
}

}
