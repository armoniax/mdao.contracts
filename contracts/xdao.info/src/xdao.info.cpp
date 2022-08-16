#include <xdao.info/xdao.info.hpp>
#include <xdaostg/xdaostg.hpp>
#include <set>
#define AMAX_TRANSFER(bank, to, quantity, memo) \
{ action(permission_level{get_self(), "active"_n }, bank, "transfer"_n, std::make_tuple( _self, to, quantity, memo )).send(); }

// ACTION xdaoinfo::createdao(const name& owner,const name& code, const string& title,
//                            const string& logo, const string& desc,
//                            const set<string>& tags, const map<name, string>& links)
// {
//     require_auth( owner );
//     CHECKC( is_account(owner) ,info_err::ACCOUNT_NOT_EXITS, "account not exits" );

//     details_t::idx_t details_sec(_self, _self.value);
//     auto details_index = details_sec.get_index<"bytitle"_n>();
//     checksum256 sec_index = HASH256(title)
//     CHECKC( details_index.find(sec_index) == details_index.end(), info_err::TITLE_REPEAT, "title already existing!" );

//     details_t detail(code);
//     CHECKC( !_db.get(detail), info_err::CODE_REPEAT, "code already existing!" );

//     detail.creator   =   owner;
//     detail.type      =   info_type::GROUP;
//     detail.status    =   info_status::RUNNING;
//     detail.title     =   title;
//     detail.logo      =   logo;
//     detail.desc	     =   desc;
//     detail.tags	     =   tags;
//     detail.links	 =   links;
//     _db.set(detail);

// }

ACTION xdaoinfo::upgradedao(name from, name to, asset quantity, string memo)
{   if (from == _self || to != _self) return;
    auto conf = _conf();
    CHECKC( quantity == conf.dao_upg_fee, info_err::INCORRECT_FEE, "incorrect handling fee" );
    auto parts = split( memo, "|" );
    CHECKC( parts.size() == 4, info_err::INVALID_FORMAT, "expected format 'code | title | desc | logo" );
    CHECKC( string(parts[0]).size() <= 12, info_err::INVALID_FORMAT, "code has more than 12 bytes");
    CHECKC( string(parts[1]).size() <= 32, info_err::INVALID_FORMAT, "title has more than 32 bytes");
    CHECKC( string(parts[2]).size() <= 128, info_err::INVALID_FORMAT, "desc has more than 128 bytes");
    CHECKC( string(parts[3]).size() <= 64, info_err::INVALID_FORMAT, "logo has more than 64 bytes");

    AMAX_TRANSFER(AMAX_TOKEN, conf.fee_taker, conf.dao_upg_fee, string("upgrade fee collection"));

    details_t::idx_t details_sec(_self, _self.value);
    auto details_index = details_sec.get_index<"bytitle"_n>();
    checksum256 sec_index = HASH256(string(parts[1]));             
    CHECKC( details_index.find(sec_index) == details_index.end(), info_err::TITLE_REPEAT, "title already existing!" );

    details_t detail((name(parts[0])));
    CHECKC( !_db.get(detail), info_err::CODE_REPEAT, "code already existing!" );

    detail.creator   =   from;
    detail.status    =   info_status::RUNNING;
    detail.type      =   info_type::DAO;
    detail.title     =   string(parts[1]);
    detail.desc      =   string(parts[2]);
    detail.logo      =   string(parts[3]);
    detail.created_at=   time_point_sec(current_time_point());

    _db.set(detail, _self);
}

ACTION xdaoinfo::updatedao(const name& owner, const name& code, const string& logo, 
                            const string& desc,const map<name, string>& links,
                            const string& symcode, string symcontract, const string& groupid)
{   
    require_auth( owner );
    auto conf = _conf();      
    
    // if(!title.empty()){
    //     details_t::idx_t details_sec(_self, _self.value);
    //     auto details_index = details_sec.get_index<"bytitle"_n>();
    //     CHECKC( details_index.find(title) == details_index.end(), info_err::TITLE_REPEAT, "title already existing!" );
    // }
 
    details_t detail(code);
    CHECKC( _db.get(detail) ,info_err::RECORD_NOT_FOUND, "record not found" );
    CHECKC( detail.creator == owner, info_err::PERMISSION_DENIED, "only the creator can operate" );
    CHECKC( conf.status != conf_status::MAINTAIN, info_err::NOT_AVAILABLE, "under maintenance" );
    CHECKC( detail.status == info_status::RUNNING, info_err::NOT_AVAILABLE, "under maintenance" );

    CHECKC( !groupid.empty(), info_err::PARAM_ERROR, "desc can not be empty" );
    CHECKC( !(symcode.empty() ^ symcontract.empty()), info_err::PARAM_ERROR, "symcode and symcontract must be null or not null" );

    bool is_expired = (detail.created_at + INFO_PERMISSION_AGING) < time_point_sec(current_time_point());
    CHECKC( ( is_account(owner) &&! is_expired )|| ( is_account(conf.managers[manager::INFO]) && is_expired ) ,info_err::PERMISSION_DENIED, "insufficient permissions" );

    if(!logo.empty())                   detail.logo   = logo;
    if(!desc.empty())                   detail.desc   = desc;
    // if(!tags.empty())                   detail.tags   = tags;
    if(!links.empty())                  detail.links  = links;
    // if(!title.empty())                  detail.title  = title;
    // if(!groupid.empty())                detail.group_id  = groupid;
    _db.set(detail, _self);
    
    if( !symcode.empty() ){

        accounts accountstable(name(symcontract), owner.value);

        const auto ac = accountstable.find(symbol_code(symcode).raw()); 
        CHECKC(ac != accountstable.end(), info_err::SYMBOL_ERROR, "symcode or symcontract not found");
        
        extended_symbol amctoken(ac->balance.symbol, name(symcontract));

        amc_info_t amcinfo(code);
        CHECKC( !_db.get(amcinfo) ,info_err::RECORD_EXITS, "record exits");
        CHECKC((amcinfo.tokens.size()+1) <= conf.amc_token_seats_max, info_err::SIZE_TOO_MUCH, "token size more than limit" );

        amcinfo.tokens.insert(amctoken);
        _db.set(amcinfo, _self);
    }
}

ACTION xdaoinfo::setstrategy(const name& owner, const name& code, const uint64_t& stgid)
{
    require_auth( owner );
    auto conf = _conf();

    details_t detail(code);
    CHECKC( _db.get(detail) ,info_err::RECORD_NOT_FOUND, "record not found");
    CHECKC( detail.creator == owner, info_err::PERMISSION_DENIED, "only the creator can operate" );
    CHECKC( conf.status != conf_status::MAINTAIN, info_err::NOT_AVAILABLE, "under maintenance" );
    CHECKC( detail.status == info_status::RUNNING, info_err::NOT_AVAILABLE, "under maintenance" );

    strategy_t::idx_t stg(XDAO_STG, XDAO_STG.value);
    CHECKC(stg.find(stgid) != stg.end(), info_err::STRATEGY_NOT_FOUND, "strategy not found");

    detail.strategy_id = stgid;
    _db.set(detail, _self);
}

ACTION xdaoinfo::binddapps(const name& owner, const name& code, const set<app_info>& dapps)
{
    require_auth( owner );
    auto conf = _conf();

    details_t detail(code);
    CHECKC( _db.get(detail) ,info_err::RECORD_NOT_FOUND, "record not found");
    CHECKC( detail.creator == owner, info_err::PERMISSION_DENIED, "only the creator can operate" );
    CHECKC( conf.status != conf_status::MAINTAIN, info_err::NOT_AVAILABLE, "under maintenance" );
    CHECKC( detail.status == info_status::RUNNING, info_err::NOT_AVAILABLE, "under maintenance" );
    CHECKC( dapps.size() != 0 ,info_err::CANNOT_ZERO, "dapp size cannot be zero" );
    CHECKC( ( detail.dapps.size() + dapps.size() ) <= conf.dapp_seats_max, info_err::SIZE_TOO_MUCH, "dapp size more than limit" );

    for( set<app_info>::iterator dapp_iter = dapps.begin(); dapp_iter!=dapps.end(); dapp_iter++ ){
        detail.dapps.insert(*dapp_iter);
    }
    _db.set(detail, _self);

}

ACTION xdaoinfo::bindevmgov(const name& owner, const name& code, const string& evmgov)
{
    require_auth( owner );
    auto conf = _conf();

    details_t detail(code);
    CHECKC( _db.get(detail) ,info_err::RECORD_NOT_FOUND, "record not found");
    CHECKC( detail.creator == owner, info_err::PERMISSION_DENIED, "only the creator can operate" );
    CHECKC( conf.status != conf_status::MAINTAIN, info_err::NOT_AVAILABLE, "under maintenance" );
    CHECKC( detail.status == info_status::RUNNING, info_err::NOT_AVAILABLE, "under maintenance" );

    evm_info_t evminfo(code);
    CHECKC( !_db.get(evminfo) ,info_err::RECORD_EXITS, "record exits");

    evminfo.evm_governance = evmgov;

    _db.set(evminfo, _self);
}

ACTION xdaoinfo::bindamcgov(const name& owner, const name& code, const uint64_t& govid)
{
    require_auth( owner );
    auto conf = _conf();

    details_t detail(code);
    CHECKC( _db.get(detail) ,info_err::RECORD_NOT_FOUND, "record not found");
    CHECKC( detail.creator == owner, info_err::PERMISSION_DENIED, "only the creator can operate" );
    CHECKC( conf.status != conf_status::MAINTAIN, info_err::NOT_AVAILABLE, "under maintenance" );
    CHECKC( detail.status == info_status::RUNNING, info_err::NOT_AVAILABLE, "under maintenance" );

    amc_info_t amcinfo(code);
    CHECKC( !_db.get(amcinfo) ,info_err::RECORD_EXITS, "record exits");

    gov_t::idx_t gov(XDAO_GOV, XDAO_GOV.value);
    CHECKC(gov.find(govid) != gov.end(), info_err::GOVERNANCE_NOT_FOUND, "governance not found");

    amcinfo.governance_id = make_optional(govid);

    _db.set(amcinfo, _self);
}

ACTION xdaoinfo::bindevmtoken(const name& owner, const name& code, const evm_symbol& evmtoken)
{
    require_auth( owner );
    auto conf = _conf();

    details_t detail(code);
    CHECKC( _db.get(detail) ,info_err::RECORD_NOT_FOUND, "record not found");
    CHECKC( detail.creator == owner, info_err::PERMISSION_DENIED, "only the creator can operate" );
    CHECKC( conf.status != conf_status::MAINTAIN, info_err::NOT_AVAILABLE, "under maintenance" );
    CHECKC( detail.status == info_status::RUNNING, info_err::NOT_AVAILABLE, "under maintenance" );

    evm_info_t evminfo(code);
    CHECKC( !_db.get(evminfo) ,info_err::RECORD_EXITS, "record exits");
    CHECKC( (evminfo.evmtokens.size()+1) <= conf.evm_token_seats_max, info_err::SIZE_TOO_MUCH, "token size more than limit" );

    evminfo.evmtokens.insert(evmtoken);
    _db.set(evminfo, _self);
}

ACTION xdaoinfo::bindamctoken(const name& owner, const name& code, const extended_symbol& amctoken)
{
    require_auth( owner );
    auto conf = _conf();

    details_t detail(code);
    CHECKC( _db.get(detail) ,info_err::RECORD_NOT_FOUND, "record not found");
    CHECKC( detail.creator == owner, info_err::PERMISSION_DENIED, "only the creator can operate" );
    CHECKC( conf.status != conf_status::MAINTAIN, info_err::NOT_AVAILABLE, "under maintenance" );
    CHECKC( detail.status == info_status::RUNNING, info_err::NOT_AVAILABLE, "under maintenance" );

    amc_info_t amcinfo(code);
    CHECKC( !_db.get(amcinfo) ,info_err::RECORD_EXITS, "record exits");
    CHECKC((amcinfo.tokens.size()+1) <= conf.amc_token_seats_max, info_err::SIZE_TOO_MUCH, "token size more than limit" );

    amcinfo.tokens.insert(amctoken);
    _db.set(amcinfo,_self);
}
//
ACTION xdaoinfo::bindevmwal(const name& owner, const name& code, const string& evmwallet, const string& chain)
{
    require_auth( owner );
    auto conf = _conf();

    details_t detail(code);
    CHECKC( _db.get(detail) ,info_err::RECORD_NOT_FOUND, "record not found");
    CHECKC( detail.creator == owner, info_err::PERMISSION_DENIED, "only the creator can operate" );
    CHECKC( conf.status != conf_status::MAINTAIN, info_err::NOT_AVAILABLE, "under maintenance" );
    CHECKC( detail.status == info_status::RUNNING, info_err::NOT_AVAILABLE, "under maintenance" );

    evm_info_t evminfo(code);
    CHECKC( !_db.get(evminfo) ,info_err::RECORD_EXITS, "record exits");

    evminfo.chain = chain;
    evminfo.evm_wallet_address = evmwallet;
    _db.set(evminfo, _self);

}

ACTION xdaoinfo::bindamcwal(const name& owner, const name& code, const uint64_t& walletid)
{
    require_auth( owner );
    auto conf = _conf();

    details_t detail(code);
    CHECKC( _db.get(detail) ,info_err::RECORD_NOT_FOUND, "record not found");
    CHECKC( detail.creator == owner, info_err::PERMISSION_DENIED, "only the creator can operate" );
    CHECKC( conf.status != conf_status::MAINTAIN, info_err::NOT_AVAILABLE, "under maintenance" );
    CHECKC( detail.status == info_status::RUNNING, info_err::NOT_AVAILABLE, "under maintenance" );

    amc_info_t amcinfo(code);
    CHECKC( !_db.get(amcinfo) ,info_err::RECORD_EXITS, "record exits");

    amcinfo.wallet_id = make_optional(walletid);
    _db.set(amcinfo, _self);

}

ACTION xdaoinfo::delamcparam(const name& owner, const name& code, set<extended_symbol> tokens)
{
    require_auth( owner );
    auto conf = _conf();

    details_t detail(code);
    CHECKC( _db.get(detail) ,info_err::RECORD_NOT_FOUND, "record not found");
    CHECKC( detail.creator == owner, info_err::PERMISSION_DENIED, "only the creator can operate" );
    CHECKC( conf.status != conf_status::MAINTAIN, info_err::NOT_AVAILABLE, "under maintenance" );
    CHECKC( detail.status == info_status::RUNNING, info_err::NOT_AVAILABLE, "under maintenance" );

    amc_info_t amcinfo(code);
    CHECKC( !_db.get(amcinfo) ,info_err::RECORD_EXITS, "record exits");

    for(set<extended_symbol>::iterator token_iter=tokens.begin();token_iter!=tokens.end();token_iter++){
        amcinfo.tokens.erase(*token_iter);
    }
    _db.set(amcinfo, _self);

}

ACTION xdaoinfo::delevmparam(const name& owner, const name& code, set<evm_symbol> tokens)
{
    require_auth( owner );
    auto conf = _conf();
    details_t detail(code);
    CHECKC( _db.get(detail) ,info_err::RECORD_NOT_FOUND, "record not found");
    CHECKC( detail.creator == owner, info_err::PERMISSION_DENIED, "only the creator can operate" );
    CHECKC( conf.status != conf_status::MAINTAIN, info_err::NOT_AVAILABLE, "under maintenance" );
    CHECKC( detail.status == info_status::RUNNING, info_err::NOT_AVAILABLE, "under maintenance" );

    evm_info_t evminfo(code);
    CHECKC( !_db.get(evminfo) ,info_err::RECORD_EXITS, "record exits");

    for( set<evm_symbol>::iterator token_iter = tokens.begin(); token_iter!=tokens.end(); token_iter++ ){
        evminfo.evmtokens.erase(*token_iter);
    }

    _db.set(evminfo, _self);
}

ACTION xdaoinfo::updatestatus(const name& code, const bool& isenable)
{
    auto conf = _conf();
    require_auth( conf.managers[manager::INFO] );

    details_t detail(code);
    CHECKC( _db.get(detail) ,info_err::RECORD_NOT_FOUND, "record not found");

    if(isenable){
        detail.status = info_status::RUNNING;
    }else{
        detail.status = info_status::BLOCK;
    }

    _db.set(detail, _self);

}

void xdaoinfo::recycledb(uint32_t max_rows) {
    require_auth( _self );
    details_t::idx_t details_tbl(_self, _self.value);
    auto detail_itr = details_tbl.begin();
    for (size_t count = 0; count < max_rows && detail_itr != details_tbl.end(); count++) {
        detail_itr = details_tbl.erase(detail_itr);
    }
}

const xdaoinfo::conf_t& xdaoinfo::_conf() {
    if (!_conf_ptr) {
        _conf_tbl_ptr = make_unique<conf_table_t>(XDAO_CONF, XDAO_CONF.value);
        CHECKC(_conf_tbl_ptr->exists(), info_err::SYSTEM_ERROR, "conf table not existed in contract" );
        _conf_ptr = make_unique<conf_t>(_conf_tbl_ptr->get());
    }
    return *_conf_ptr;
}

