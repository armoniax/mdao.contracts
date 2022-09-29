#include <mdao.token/mdao.token.hpp>
#include "mdao.info/mdao.info.hpp"
#include <set>
#include <thirdparty/utils.hpp>

#define AMAX_TRANSFER(bank, to, quantity, memo) \
{ action(permission_level{get_self(), "active"_n }, bank, "transfer"_n, std::make_tuple( _self, to, quantity, memo )).send(); }

ACTION mdaoinfo::onupgradedao(name from, name to, asset quantity, string memo)
{   if (from == _self || to != _self) return;
    auto conf = _conf();
    CHECKC( quantity >= conf.upgrade_fee, info_err::INCORRECT_FEE, "incorrect handling fee" );

    auto parts = split( memo, "|" );
    CHECKC( parts.size() == 4, info_err::INVALID_FORMAT, "expected format: 'code | title | desc | logo" );
    
    string_view code = string_view(parts[0]);
    bool is_not_contain_amax    = find_substr(code, amax_limit)    == -1 ? true : false;
    bool is_not_contain_aplink  = find_substr(code, aplink_limit)  == -1 ? true : false;
    bool is_not_contain_armonia = find_substr(code, armonia_limit) == -1 ? true : false;
    bool is_not_contain_meta    = find_substr(code, meta_limit)    == -1 ? true : false;
    CHECKC( (conf.admin == from) || (conf.admin != from && is_not_contain_amax && is_not_contain_aplink && is_not_contain_armonia && is_not_contain_meta), 
                     info_err::INVALID_FORMAT, "code cannot include aplink,amax,armonia,meta");
    CHECKC( (code.size() == 12) || (conf.admin == from && code.size() <= 12), info_err::INVALID_FORMAT, "code length is more than 12 bytes");

    string_view title = string_view(parts[1]);
    CHECKC( title.size() <= 32, info_err::INVALID_FORMAT, "title length is more than 32 bytes");

    string_view desc = string_view(parts[2]);
    CHECKC( desc.size() <= 128, info_err::INVALID_FORMAT, "desc length is more than 128 bytes");

    string_view logo = string_view(parts[3]);
    CHECKC( logo.size() <= 64, info_err::INVALID_FORMAT, "logo length is more than 64 bytes");

    AMAX_TRANSFER(AMAX_TOKEN, conf.fee_taker, quantity, string("upgrade fee collection"));

    dao_info_t::idx_t info_sec(_self, _self.value);
    auto info_index = info_sec.get_index<"bytitle"_n>();
    checksum256 sec_index = HASH256(string(parts[1]));             
    CHECKC( info_index.find(sec_index) == info_index.end(), info_err::TITLE_REPEAT, "title already existing!" );

    dao_info_t info((name(code)));
    CHECKC( !_db.get(info), info_err::CODE_REPEAT, "code already existing!" );

    info.creator   =   from;
    info.status    =   info_status::RUNNING;
    info.title     =   title;
    info.desc      =   desc;
    info.logo      =   logo;
    info.created_at=   current_time_point();

    _db.set(info, _self);
}

ACTION mdaoinfo::updatedao(const name& owner, const name& code, const string& logo, 
                            const string& desc,const map<name, string>& links,
                            const string& sym_code, string sym_contract, const string& group_id)
{   
    require_auth( owner );
    auto conf = _conf();      
    CHECKC( conf.status != conf_status::PENDING, info_err::NOT_AVAILABLE, "under maintenance" );
 
    dao_info_t info(code);
    CHECKC( _db.get(info) ,info_err::RECORD_NOT_FOUND, "record not found" );
    CHECKC( info.creator == owner, info_err::PERMISSION_DENIED, "only the creator can operate" );
    CHECKC( info.status == info_status::RUNNING, info_err::NOT_AVAILABLE, "under maintenance" );

    CHECKC( sym_code.empty() == sym_contract.empty(), info_err::PARAM_ERROR, "symcode and symcontract must be null or not null" );

    // bool is_expired = (detail.created_at + INFO_PERMISSION_AGING) < time_point_sec(current_time_point());
    // CHECKC( ( has_auth(owner) &&! is_expired )|| ( has_auth(conf.managers[manager::INFO]) && is_expired ) ,info_err::PERMISSION_DENIED, "insufficient permissions" );

    if(!logo.empty())                   info.logo   = logo;
    if(!desc.empty())                   info.desc   = desc;
    if(!links.empty())                  info.resource_links  = links;
    if(!group_id.empty())               info.group_id  = group_id;
    
    if( !sym_code.empty() ){

        stats statstable(name(sym_contract), symbol_code(sym_code).raw());
        const auto st = statstable.find(symbol_code(sym_code).raw());
        CHECKC(st != statstable.end(), info_err::SYMBOL_ERROR, "symcode or symcontract not found");
        CHECKC(st->issuer == owner, info_err::PERMISSION_DENIED, "the owner is not a token creator");

        accounts accountstable(name(sym_contract), owner.value);
        const auto ac = accountstable.find(symbol_code(sym_code).raw()); 
        CHECKC(ac != accountstable.end(), info_err::SYMBOL_ERROR, "symcode or symcontract not found");
        
        extended_symbol token(ac->balance.symbol, name(sym_contract));

        info.token = token;
    }

    _db.set(info, _self);
}

ACTION mdaoinfo::deldao(const name& admin, const name& code)
{   
    require_auth( admin );
    auto conf = _conf();      
    CHECKC( conf.status != conf_status::PENDING, info_err::NOT_AVAILABLE, "under maintenance" );
    CHECKC( conf.admin == admin, info_err::PERMISSION_DENIED, "only the admin can operate" );

    dao_info_t info(code);
    CHECKC( _db.get(info) ,info_err::RECORD_NOT_FOUND, "record not found" ); 

    _db.del(info);
}

ACTION mdaoinfo::transferdao(const name& owner, const name& code, const name& receiver)
{   
    require_auth( owner );
    auto conf = _conf();      
    CHECKC( conf.status != conf_status::PENDING, info_err::NOT_AVAILABLE, "under maintenance" );
    CHECKC( is_account(receiver), info_err::ACCOUNT_NOT_EXITS, "receiver does not exist" );

    dao_info_t info(code);
    CHECKC( _db.get(info) ,info_err::RECORD_NOT_FOUND, "record not found" );
    CHECKC( info.creator == owner, info_err::PERMISSION_DENIED, "only the creator can operate" );
    CHECKC( info.status == info_status::RUNNING, info_err::NOT_AVAILABLE, "under maintenance" );
    
    info.creator = receiver;
    _db.set(info, _self);
}

ACTION mdaoinfo::updatecode(const name& admin, const name& code, const name& new_code)
{   
    require_auth( admin );
    auto conf = _conf();      
    CHECKC( conf.status != conf_status::PENDING, info_err::NOT_AVAILABLE, "under maintenance" );
    CHECKC( conf.admin == admin, info_err::PERMISSION_DENIED, "only the admin can operate" );
    
    dao_info_t::idx_t info_tbl(get_self(), get_self().value);
    const auto info_itr = info_tbl.find(code.value);
    CHECKC( info_itr != info_tbl.end(),info_err::RECORD_NOT_FOUND, "record not found" );
    CHECKC( info_itr->status == info_status::RUNNING, info_err::NOT_AVAILABLE, "under maintenance" );

    dao_info_t new_info(new_code);
    CHECKC( !_db.get(new_info) ,info_err::RECORD_EXITS, "new code is already exists" );
    
    info_tbl.modify(info_itr, get_self(), [&](auto &i) {
            i.dao_code = new_code;
    });
}

ACTION mdaoinfo::binddapps(const name& owner, const name& code, const set<app_info>& dapps)
{
    require_auth( owner );
    auto conf = _conf();
    CHECKC( conf.status != conf_status::PENDING, info_err::NOT_AVAILABLE, "under maintenance" );

    dao_info_t info(code);
    CHECKC( _db.get(info) ,info_err::RECORD_NOT_FOUND, "record not found");
    CHECKC( info.creator == owner, info_err::PERMISSION_DENIED, "only the creator can operate" );
    CHECKC( info.status == info_status::RUNNING, info_err::NOT_AVAILABLE, "under maintenance" );
    CHECKC( dapps.size() != 0 ,info_err::CANNOT_ZERO, "dapp size cannot be zero" );
    CHECKC( ( info.dapps.size() + dapps.size() ) <= conf.dapp_seats_max, info_err::SIZE_TOO_MUCH, "dapp size more than limit" );

    for( set<app_info>::iterator dapp_iter = dapps.begin(); dapp_iter!=dapps.end(); dapp_iter++ ){
        info.dapps.insert(*dapp_iter);
    }
    _db.set(info, _self);

}

// ACTION mdaoinfo::bindgov(const name& owner, const name& code, const uint64_t& govid)
// {
//     CHECKC( false, info_err::NOT_AVAILABLE, "under maintenance" );

//     require_auth( owner );
//     auto conf = _conf();

//     dao_info_t info(code);
//     CHECKC( _db.get(info) ,info_err::RECORD_NOT_FOUND, "record not found");
//     CHECKC( info.creator == owner, info_err::PERMISSION_DENIED, "only the creator can operate" );
//     CHECKC( conf.status != conf_status::PENDING, info_err::NOT_AVAILABLE, "under maintenance" );
//     CHECKC( info.status == info_status::RUNNING, info_err::NOT_AVAILABLE, "under maintenance" );

//     gov_t::idx_t gov(MDAO_GOV, MDAO_GOV.value);
//     CHECKC(gov.find(govid) != gov.end(), info_err::GOVERNANCE_NOT_FOUND, "governance not found");

//     _db.set(info, _self);
       
// }

ACTION mdaoinfo::bindtoken(const name& owner, const name& code, const extended_symbol& token)
{
    require_auth( owner );
    auto conf = _conf();
    CHECKC( conf.status != conf_status::PENDING, info_err::NOT_AVAILABLE, "under maintenance" );

    dao_info_t info(code);
    CHECKC( _db.get(info) ,info_err::RECORD_NOT_FOUND, "record not found");
    CHECKC( info.creator == owner, info_err::PERMISSION_DENIED, "only the creator can operate" );
    CHECKC( info.status == info_status::RUNNING, info_err::NOT_AVAILABLE, "under maintenance" );

    info.token = token; 
}

ACTION mdaoinfo::updatestatus(const name& code, const bool& is_enable)
{
    auto conf = _conf();
    require_auth( conf.managers[manager_type::INFO] );

    dao_info_t info(code);
    CHECKC( _db.get(info) ,info_err::RECORD_NOT_FOUND, "record not found");

    info.status = is_enable ? info_status::RUNNING : info_status::BLOCK;
    _db.set(info, _self);

}

void mdaoinfo::recycledb(uint32_t max_rows) {
    require_auth( _self );
    dao_info_t::idx_t info_tbl(_self, _self.value);
    auto info_itr = info_tbl.begin();
    for (size_t count = 0; count < max_rows && info_itr != info_tbl.end(); count++) {
        info_itr = info_tbl.erase(info_itr);
    }
}

// ACTION mdaoinfo::createtoken(const name& code, const name& owner, const uint16_t& taketranratio, 
//                             const uint16_t& takegasratio, const string& fullname, const asset& maximum_supply)
// {
//     CHECKC( false, info_err::NOT_AVAILABLE, "under maintenance" );

//     auto conf = _conf();
//     require_auth( owner );

//     CHECKC( fullname.size() <= 20, info_err::SIZE_TOO_MUCH, "fullname has more than 20 bytes")
//     CHECKC( maximum_supply.amount > 0, info_err::NOT_POSITIVE, "not positive quantity:" + maximum_supply.to_string() )
//     symbol_code supply_code = maximum_supply.symbol.code();
//     CHECKC( supply_code.length() > 3, info_err::NO_AUTH, "cannot create limited token" )
//     CHECKC( !conf.black_symbols.count(supply_code) ,info_err::NOT_ALLOW, "token not allowed to create" );

//     stats statstable( MDAO_TOKEN, supply_code.raw() );
//     CHECKC( statstable.find(supply_code.raw()) == statstable.end(), info_err::CODE_REPEAT, "token already exist")

//     dao_info_t info(code);
//     CHECKC( _db.get(info) ,info_err::RECORD_NOT_FOUND, "record not found" );
//     CHECKC( info.creator == owner ,info_err::PERMISSION_DENIED, "only the creator can operate" );

//     XTOKEN_CREATE_TOKEN(MDAO_TOKEN, owner, maximum_supply, taketranratio, takegasratio, fullname)

//     info.token = extended_symbol(maximum_supply.symbol, MDAO_TOKEN); 
//     _db.set(info, _self);

// }

const mdaoinfo::conf_t& mdaoinfo::_conf() {
    if (!_conf_ptr) {
        _conf_tbl_ptr = make_unique<conf_table_t>(MDAO_CONF, MDAO_CONF.value);
        CHECKC(_conf_tbl_ptr->exists(), info_err::SYSTEM_ERROR, "conf table not existed in contract" );
        _conf_ptr = make_unique<conf_t>(_conf_tbl_ptr->get());
    }
    return *_conf_ptr;
}

