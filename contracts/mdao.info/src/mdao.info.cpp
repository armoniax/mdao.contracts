#include <mdao.info/mdao.info.hpp>
#include <mdao.stg/mdao.stg.hpp>
#include <mdao.token/mdao.token.hpp>
#include <set>

#define AMAX_TRANSFER(bank, to, quantity, memo) \
{ action(permission_level{get_self(), "active"_n }, bank, "transfer"_n, std::make_tuple( _self, to, quantity, memo )).send(); }

ACTION mdaoinfo::upgradedao(name from, name to, asset quantity, string memo)
{   if (from == _self || to != _self) return;
    auto conf = _conf();
    CHECKC( quantity == conf.dao_upgrade_fee, info_err::INCORRECT_FEE, "incorrect handling fee" );
    auto parts = split( memo, "|" );
    CHECKC( parts.size() == 4, info_err::INVALID_FORMAT, "expected format 'code | title | desc | logo" );
    CHECKC( (string(parts[0]).size() == 12) || (conf.dao_admin == from && string(parts[0]).size() <= 12), info_err::INVALID_FORMAT, "code has more than 12 bytes");
    CHECKC( string(parts[1]).size() <= 32, info_err::INVALID_FORMAT, "title has more than 32 bytes");
    CHECKC( string(parts[2]).size() <= 128, info_err::INVALID_FORMAT, "desc has more than 128 bytes");
    CHECKC( string(parts[3]).size() <= 64, info_err::INVALID_FORMAT, "logo has more than 64 bytes");

    AMAX_TRANSFER(AMAX_TOKEN, conf.fee_taker, conf.dao_upgrade_fee, string("upgrade fee collection"));

    details_t::idx_t details_sec(_self, _self.value);
    auto details_index = details_sec.get_index<"bytitle"_n>();
    checksum256 sec_index = HASH256(string(parts[1]));             
    CHECKC( details_index.find(sec_index) == details_index.end(), info_err::TITLE_REPEAT, "title already existing!" );

    details_t detail((name(parts[0])));
    CHECKC( !_db.get(detail), info_err::CODE_REPEAT, "code already existing!" );

    detail.creator   =   from;
    detail.status    =   info_status::RUNNING;
    detail.title     =   string(parts[1]);
    detail.desc      =   string(parts[2]);
    detail.logo      =   string(parts[3]);
    detail.created_at=   time_point_sec(current_time_point());

    _db.set(detail, _self);
}

ACTION mdaoinfo::updatedao(const name& owner, const name& code, const string& logo, 
                            const string& desc,const map<name, string>& links,
                            const string& symcode, string symcontract, const string& groupid)
{   
    require_auth( owner );
    auto conf = _conf();      
    CHECKC( conf.status != conf_status::MAINTAIN, info_err::NOT_AVAILABLE, "under maintenance" );
 
    details_t detail(code);
    CHECKC( _db.get(detail) ,info_err::RECORD_NOT_FOUND, "record not found" );
    CHECKC( detail.creator == owner, info_err::PERMISSION_DENIED, "only the creator can operate" );
    CHECKC( detail.status == info_status::RUNNING, info_err::NOT_AVAILABLE, "under maintenance" );

    CHECKC( symcode.empty() == symcontract.empty(), info_err::PARAM_ERROR, "symcode and symcontract must be null or not null" );

    // bool is_expired = (detail.created_at + INFO_PERMISSION_AGING) < time_point_sec(current_time_point());
    // CHECKC( ( has_auth(owner) &&! is_expired )|| ( has_auth(conf.managers[manager::INFO]) && is_expired ) ,info_err::PERMISSION_DENIED, "insufficient permissions" );

    if(!logo.empty())                   detail.logo   = logo;
    if(!desc.empty())                   detail.desc   = desc;
    if(!links.empty())                  detail.links  = links;
    if(!groupid.empty())                detail.group_id  = groupid;
    
    if( !symcode.empty() ){

        accounts accountstable(name(symcontract), owner.value);

        const auto ac = accountstable.find(symbol_code(symcode).raw()); 
        CHECKC(ac != accountstable.end(), info_err::SYMBOL_ERROR, "symcode or symcontract not found");
        
        extended_symbol token(ac->balance.symbol, name(symcontract));

        CHECKC((detail.tokens.size()+1) <= conf.token_seats_max, info_err::SIZE_TOO_MUCH, "token size more than limit" );
        detail.tokens.push_back(token);         
        
    }

    _db.set(detail, _self);
}

ACTION mdaoinfo::setstrategy(const name& owner, const name& code, const name& stgtype, const uint64_t& stgid)
{
    require_auth( owner );
    auto conf = _conf();

    details_t detail(code);
    CHECKC( _db.get(detail) ,info_err::RECORD_NOT_FOUND, "record not found");
    CHECKC( detail.creator == owner, info_err::PERMISSION_DENIED, "only the creator can operate" );
    CHECKC( conf.status != conf_status::MAINTAIN, info_err::NOT_AVAILABLE, "under maintenance" );
    CHECKC( detail.status == info_status::RUNNING, info_err::NOT_AVAILABLE, "under maintenance" );

    strategy_t::idx_t stg(MDAO_STG, MDAO_STG.value);
    CHECKC(stg.find(stgid) != stg.end(), info_err::STRATEGY_NOT_FOUND, "strategy not found");
    detail.strategys[stgtype] = stgid;

    _db.set(detail, _self);
}

ACTION mdaoinfo::binddapps(const name& owner, const name& code, const set<app_info>& dapps)
{
    require_auth( owner );
    auto conf = _conf();
    CHECKC( conf.status != conf_status::MAINTAIN, info_err::NOT_AVAILABLE, "under maintenance" );

    details_t detail(code);
    CHECKC( _db.get(detail) ,info_err::RECORD_NOT_FOUND, "record not found");
    CHECKC( detail.creator == owner, info_err::PERMISSION_DENIED, "only the creator can operate" );
    CHECKC( detail.status == info_status::RUNNING, info_err::NOT_AVAILABLE, "under maintenance" );
    CHECKC( dapps.size() != 0 ,info_err::CANNOT_ZERO, "dapp size cannot be zero" );
    CHECKC( ( detail.dapps.size() + dapps.size() ) <= conf.dapp_seats_max, info_err::SIZE_TOO_MUCH, "dapp size more than limit" );

    for( set<app_info>::iterator dapp_iter = dapps.begin(); dapp_iter!=dapps.end(); dapp_iter++ ){
        detail.dapps.insert(*dapp_iter);
    }
    _db.set(detail, _self);

}

ACTION mdaoinfo::bindgov(const name& owner, const name& code, const uint64_t& govid)
{
    CHECKC( false, info_err::NOT_AVAILABLE, "under maintenance" );

    require_auth( owner );
    auto conf = _conf();

    details_t detail(code);
    CHECKC( _db.get(detail) ,info_err::RECORD_NOT_FOUND, "record not found");
    CHECKC( detail.creator == owner, info_err::PERMISSION_DENIED, "only the creator can operate" );
    CHECKC( conf.status != conf_status::MAINTAIN, info_err::NOT_AVAILABLE, "under maintenance" );
    CHECKC( detail.status == info_status::RUNNING, info_err::NOT_AVAILABLE, "under maintenance" );

    gov_t::idx_t gov(MDAO_GOV, MDAO_GOV.value);
    CHECKC(gov.find(govid) != gov.end(), info_err::GOVERNANCE_NOT_FOUND, "governance not found");


    _db.set(detail, _self);
       
}

ACTION mdaoinfo::bindtoken(const name& owner, const name& code, const extended_symbol& token)
{
    require_auth( owner );
    auto conf = _conf();
    CHECKC( conf.status != conf_status::MAINTAIN, info_err::NOT_AVAILABLE, "under maintenance" );

    details_t detail(code);
    CHECKC( _db.get(detail) ,info_err::RECORD_NOT_FOUND, "record not found");
    CHECKC( detail.creator == owner, info_err::PERMISSION_DENIED, "only the creator can operate" );
    CHECKC( detail.status == info_status::RUNNING, info_err::NOT_AVAILABLE, "under maintenance" );

    CHECKC((detail.tokens.size()+1) <= conf.token_seats_max, info_err::SIZE_TOO_MUCH, "token size more than limit" );
    detail.tokens.push_back(token); 
}

ACTION mdaoinfo::bindwal(const name& owner, const name& code, const uint64_t& walletid)
{
    CHECKC( false, info_err::NOT_AVAILABLE, "under maintenance" );

    require_auth( owner );
    auto conf = _conf();
    CHECKC( conf.status != conf_status::MAINTAIN, info_err::NOT_AVAILABLE, "under maintenance" );

    details_t detail(code);
    CHECKC( _db.get(detail) ,info_err::RECORD_NOT_FOUND, "record not found");
    CHECKC( detail.creator == owner, info_err::PERMISSION_DENIED, "only the creator can operate" );
    CHECKC( detail.status == info_status::RUNNING, info_err::NOT_AVAILABLE, "under maintenance" );

    _db.set( detail, _self);

}

ACTION mdaoinfo::delparam(const name& owner, const name& code, vector<extended_symbol> tokens)
{
    require_auth( owner );
    auto conf = _conf();
    CHECKC( conf.status != conf_status::MAINTAIN, info_err::NOT_AVAILABLE, "under maintenance" );

    details_t detail(code);
    CHECKC( _db.get(detail) ,info_err::RECORD_NOT_FOUND, "record not found");
    CHECKC( detail.creator == owner, info_err::PERMISSION_DENIED, "only the creator can operate" );
    CHECKC( detail.status == info_status::RUNNING, info_err::NOT_AVAILABLE, "under maintenance" );

    for(vector<extended_symbol>::iterator token_iter=tokens.begin();token_iter!=tokens.end();token_iter++){
        detail.tokens.erase(token_iter);
    }
    _db.set( detail, _self);

}

ACTION mdaoinfo::updatestatus(const name& code, const bool& isenable)
{
    auto conf = _conf();
    require_auth( conf.managers[manager_type::INFO] );

    details_t detail(code);
    CHECKC( _db.get(detail) ,info_err::RECORD_NOT_FOUND, "record not found");

    detail.status = isenable ? info_status::RUNNING : info_status::BLOCK;
    _db.set(detail, _self);

}

void mdaoinfo::recycledb(uint32_t max_rows) {
    require_auth( _self );
    details_t::idx_t details_tbl(_self, _self.value);
    auto detail_itr = details_tbl.begin();
    for (size_t count = 0; count < max_rows && detail_itr != details_tbl.end(); count++) {
        detail_itr = details_tbl.erase(detail_itr);
    }
}

ACTION mdaoinfo::createtoken(const name& code, const name& owner, const uint16_t& taketranratio, 
                            const uint16_t& takegasratio, const string& fullname, const asset& maximum_supply)
{
    CHECKC( false, info_err::NOT_AVAILABLE, "under maintenance" );

    auto conf = _conf();
    require_auth( owner );

    CHECKC( fullname.size() <= 20, info_err::SIZE_TOO_MUCH, "fullname has more than 20 bytes")
    CHECKC( maximum_supply.amount > 0, info_err::NOT_POSITIVE, "not positive quantity:" + maximum_supply.to_string() )
    symbol_code supply_code = maximum_supply.symbol.code();
    CHECKC( supply_code.length() > 3, info_err::NO_AUTH, "cannot create limited token" )
    CHECKC( !conf.limited_symbols.count(supply_code) ,info_err::NOT_ALLOW, "token not allowed to create" );

    stats statstable( MDAO_TOKEN, supply_code.raw() );
    CHECKC( statstable.find(supply_code.raw()) == statstable.end(), info_err::CODE_REPEAT, "token already exist")

    details_t detail(code);
    CHECKC( _db.get(detail) ,info_err::RECORD_NOT_FOUND, "record not found" );
    CHECKC( detail.creator == owner ,info_err::PERMISSION_DENIED, "only the creator can operate" );
    CHECKC((detail.tokens.size()+1) <= conf.token_seats_max, info_err::SIZE_TOO_MUCH, "token size more than limit" );

    XTOKEN_CREATE_TOKEN(MDAO_TOKEN, owner, maximum_supply, taketranratio, takegasratio, fullname)

    detail.tokens.push_back(extended_symbol(maximum_supply.symbol, MDAO_TOKEN)); 
    _db.set(detail, _self);

}

const mdaoinfo::conf_t& mdaoinfo::_conf() {
    if (!_conf_ptr) {
        _conf_tbl_ptr = make_unique<conf_table_t>(MDAO_CONF, MDAO_CONF.value);
        CHECKC(_conf_tbl_ptr->exists(), info_err::SYSTEM_ERROR, "conf table not existed in contract" );
        _conf_ptr = make_unique<conf_t>(_conf_tbl_ptr->get());
    }
    return *_conf_ptr;
}

