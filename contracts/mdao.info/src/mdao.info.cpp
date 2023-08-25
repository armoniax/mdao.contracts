#include "mdao.info/mdao.info.hpp"
#include <set>
#include <thirdparty/utils.hpp>
#include <amax.ntoken/did.ntoken_db.hpp>

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

    // auto did_acnts = amax::account_t::idx_t( did::DID_NTOKEN, from.value );
    // bool is_auth = false;
    // for( auto did_acnts_iter = did_acnts.begin(); did_acnts_iter!=did_acnts.end(); did_acnts_iter++ ){
    //     if(did_acnts_iter->balance.amount > 0){
    //         is_auth = true;
    //         break;
    //     }
    // }
    // CHECKC( is_auth, info_err::DID_NOT_AUTH, "did is not authenticated" );

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
    auto conf = _conf();
    CHECKC( conf.status != conf_status::PENDING, info_err::NOT_AVAILABLE, "under maintenance" );

    dao_info_t info(code);
    _check_permission(info, code, owner, conf);

    CHECKC( sym_code.empty() == sym_contract.empty(), info_err::PARAM_ERROR, "symcode and symcontract must be null or not null" );

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

ACTION mdaoinfo::setlogo(const name& owner, const name& code, const string& logo)
{
    require_auth( owner );

    auto conf = _conf();
    CHECKC( conf.status != conf_status::PENDING, info_err::NOT_AVAILABLE, "under maintenance" );

    dao_info_t info(code);
    CHECKC( _db.get(info) ,info_err::RECORD_NOT_FOUND, "record not found" );
    CHECKC( info.creator == owner, info_err::PERMISSION_DENIED, "only the creator can operate" );
    CHECKC( info.status == info_status::RUNNING, info_err::NOT_AVAILABLE, "under maintenance" );
    info.logo = logo;
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

    dao_info_t info(code);
    CHECKC( _db.get(info) ,info_err::RECORD_NOT_FOUND, "record not found" );
    CHECKC( info.status == info_status::RUNNING, info_err::NOT_AVAILABLE, "under maintenance" );

    dao_info_t new_info(new_code);
    CHECKC( !_db.get(new_info) ,info_err::RECORD_EXITS, "new code is already exists" );

    info.dao_code = new_code;
    _db.set(info, _self);

    info.dao_code = code;
    _db.del(info);

}

ACTION mdaoinfo::binddapps(const name& owner, const name& code, const set<app_info>& dapps)
{
    // require_auth( owner );
    auto conf = _conf();
    CHECKC( conf.status != conf_status::PENDING, info_err::NOT_AVAILABLE, "under maintenance" );

    dao_info_t info(code);
    _check_permission(info, code, owner, conf);
    CHECKC( dapps.size() != 0 ,info_err::CANNOT_ZERO, "dapp size cannot be zero" );
    CHECKC( ( info.dapps.size() + dapps.size() ) <= conf.dapp_seats_max, info_err::SIZE_TOO_MUCH, "dapp size more than limit" );

    for( set<app_info>::iterator dapp_iter = dapps.begin(); dapp_iter!=dapps.end(); dapp_iter++ ){
        info.dapps.insert(*dapp_iter);
    }
    _db.set(info, _self);

}

ACTION mdaoinfo::bindtoken(const name& owner, const name& code, const extended_symbol& token)
{
    // require_auth( owner );
    auto conf = _conf();
    CHECKC( conf.status != conf_status::PENDING, info_err::NOT_AVAILABLE, "under maintenance" );

    dao_info_t info(code);
    _check_permission(info, code, owner, conf);

    info.token = token;
    _db.set(info, _self);
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
void mdaoinfo::settags(const name& code, map<name, tags_info>& tags) {
    auto conf = _conf();
    auto conf2 = _conf2();

    dao_info_t info(code);
    CHECKC( _db.get(info) ,info_err::RECORD_NOT_FOUND, "record not found");

    map<name, tags_info>::iterator iter;
    for(iter = tags.begin(); iter != tags.end(); iter++){
        name tag_code = iter->first;
        tags_info tags_i = iter->second;

        vector<string> tag_list = tags_i.tags;
        vector<string> info_tags = info.tags.at(tag_code).tags;

        switch (tag_code.value)
        {
            case tags_code::OFFICIAL.value:{
                CHECKC( has_auth(conf.managers[manager_type::INFO]), info_err::PERMISSION_DENIED, "permission denied" );
                break;
            }
            case tags_code::OPTIONAL.value:{
                CHECKC( has_auth(info.creator), info_err::PERMISSION_DENIED, "permission denied");
                CHECKC( info_tags.size() + tag_list.size() < 4, info_err::PARAM_ERROR, "tags count over limit" );
                break;
            }
            case tags_code::LANGUAGE.value:{
                CHECKC( has_auth(info.creator), info_err::PERMISSION_DENIED, "permission denied" );
                CHECKC( info_tags.size() + tag_list.size() < 2, info_err::PARAM_ERROR, "tags count over limit" );
                break;
            }
            default:
                CHECKC( false, info_err::PARAM_ERROR, "tag code error");
        }

        for( vector<string>::iterator tag_iter = tag_list.begin(); tag_iter!=tag_list.end(); tag_iter++ ){
            vector<string> conf_tags = conf2.available_tags.at(tag_code).tags;
            CHECKC( count(conf_tags.begin(), conf_tags.end(), *tag_iter) > 0, info_err::PARAM_ERROR, "unsupport tag" );

            if( count(info_tags.begin(), info_tags.end(), *tag_iter) > 0 ){
                continue;
            }

            info_tags.push_back( *tag_iter);
        }

        info.tags[tag_code].tags = info_tags;
        _db.set(info, _self);
    }

}

void mdaoinfo::deltag(const name& code, const string& tag) {
    auto parts = split( tag, "." );
    CHECKC( parts.size() == 2, info_err::INVALID_FORMAT, "invalid format" );

    dao_info_t info(code);
    CHECKC( _db.get(info) ,info_err::RECORD_NOT_FOUND, "record not found");

    name tag_code = name(parts[0]);
    vector<string> info_tags = info.tags.at(tag_code).tags;

    switch (tag_code.value)
    {
        case tags_code::OFFICIAL.value:{
            auto conf = _conf();
            CHECKC( has_auth(conf.managers[manager_type::INFO]), info_err::PERMISSION_DENIED, "permission denied" );
            break;
        }
        case tags_code::OPTIONAL.value:
        case tags_code::LANGUAGE.value:{
            CHECKC( has_auth(info.creator), info_err::PERMISSION_DENIED, "permission denied3" );
            break;
        }
        default:
            CHECKC( false, info_err::PARAM_ERROR, "tag code error");
    }

    bool is_exist = false;
    for (vector<string>::iterator iter = info_tags.begin(); iter!=info_tags.end(); iter++) {
        if ( *iter == tag ){
            info_tags.erase(iter);
            is_exist = true;
            break;
        }
    }
    CHECKC( is_exist, info_err::PARAM_ERROR, "tag not found");

    info.tags[tag_code].tags = info_tags;
    _db.set(info, _self);
}

void mdaoinfo::replacetag(const name& code, map<name, tags_info>& tags) {
    auto conf = _conf();
    auto conf2 = _conf2();

    dao_info_t info(code);
    CHECKC( _db.get(info) ,info_err::RECORD_NOT_FOUND, "record not found");

    map<name, tags_info>::iterator iter;
    for(iter = tags.begin(); iter != tags.end(); iter++){
        name tag_code = iter->first;
        tags_info tags_i = iter->second;

        vector<string> tag_list = tags_i.tags;

        switch (tag_code.value)
        {
            case tags_code::OFFICIAL.value:{
                CHECKC( has_auth(conf.managers[manager_type::INFO]), info_err::PERMISSION_DENIED, "permission denied" );
                break;
            }
            case tags_code::OPTIONAL.value:{
                CHECKC( has_auth(info.creator), info_err::PERMISSION_DENIED, "permission denied");
                CHECKC( tag_list.size() < 4, info_err::PARAM_ERROR, "tags count over limit" );
                break;
            }
            case tags_code::LANGUAGE.value:{
                CHECKC( has_auth(info.creator), info_err::PERMISSION_DENIED, "permission denied" );
                CHECKC( tag_list.size() < 2, info_err::PARAM_ERROR, "tags count over limit" );
                break;
            }
            default:
                CHECKC( false, info_err::PARAM_ERROR, "tag code error");
        }

        for( vector<string>::iterator tag_iter = tag_list.begin(); tag_iter!=tag_list.end(); tag_iter++ ){
            vector<string> conf_tags = conf2.available_tags.at(tag_code).tags;
            CHECKC( count(conf_tags.begin(), conf_tags.end(), *tag_iter) > 0, info_err::PARAM_ERROR, "unsupport tag" );
            CHECKC( count(tag_list.begin(), tag_list.end(), *tag_iter) == 1 , info_err::PARAM_ERROR, "parameter has duplicate tag" );
        }
    }
    info.tags.clear();
    info.tags = tags;
    _db.set(info, _self);
}

// void mdaoinfo::recycledb(uint32_t max_rows) {
//     require_auth( _self );
//     dao_info_t::idx_t info_tbl(_self, _self.value);
//     auto info_itr = info_tbl.begin();
//     for (size_t count = 0; count < max_rows && info_itr != info_tbl.end(); count++) {
//         info_itr = info_tbl.erase(info_itr);
//     }
// }

// ACTION mdaoinfo::createtoken(const name& code, const name& owner, const uint16_t& transfer_ratio,
//                              const string& fullname, const asset& maximum_supply, const string& metadata)
// {
//     CHECKC( false, info_err::NOT_AVAILABLE, "under maintenance" );

//     auto conf = _conf();
//     CHECKC( conf.status != conf_status::PENDING, info_err::NOT_AVAILABLE, "under maintenance" );

//     CHECKC( fullname.size() <= 20, info_err::SIZE_TOO_MUCH, "fullname has more than 20 bytes")
//     CHECKC( maximum_supply.amount > 0, info_err::NOT_POSITIVE, "not positive quantity:" + maximum_supply.to_string() )
//     symbol_code supply_code = maximum_supply.symbol.code();
//     CHECKC( supply_code.length() > 3, info_err::NO_AUTH, "cannot create limited token" )
//     CHECKC( !conf.black_symbols.count(supply_code) ,info_err::NOT_ALLOW, "token not allowed to create" );

//     stats statstable( MDAO_TOKEN, supply_code.raw() );
//     CHECKC( statstable.find(supply_code.raw()) == statstable.end(), info_err::CODE_REPEAT, "token already exist")

//      dao_info_t info(code);
//      _check_permission(info, code, owner, conf);

//     XTOKEN_CREATE_TOKEN(MDAO_TOKEN, owner, maximum_supply, transfer_ratio, fullname, code, metadata)

//     info.token = extended_symbol(maximum_supply.symbol, MDAO_TOKEN);
//     _db.set(info, _self);

// }

// ACTION mdaoinfo::issuetoken(const name& owner, const name& code, const name& to,
//                             const asset& quantity, const string& memo)
// {
//     CHECKC( false, info_err::NOT_AVAILABLE, "under maintenance" );

//     auto conf = _conf();

//     symbol_code supply_code = quantity.symbol.code();
//     stats statstable( MDAO_TOKEN, supply_code.raw() );
//     CHECKC( statstable.find(supply_code.raw()) != statstable.end(), info_err::TOKEN_NOT_EXIST, "token not exist")

//      dao_info_t info(code);
//      _check_permission(info, code, owner, conf);

//     XTOKEN_ISSUE(MDAO_TOKEN, to, quantity, memo)

// }

ACTION mdaoinfo::bindntoken(const name& owner, const name& code, const extended_nsymbol& ntoken)
{
    // require_auth( owner );
    auto conf = _conf();
    CHECKC( conf.status != conf_status::PENDING, info_err::NOT_AVAILABLE, "under maintenance" );

    dao_info_t info(code);
    _check_permission(info, code, owner, conf);

    info.ntoken = ntoken;
}

void mdaoinfo::_check_permission( dao_info_t& info, const name& code, const name& owner,  const conf_t& conf ) {
    CHECKC( _db.get(info) ,info_err::RECORD_NOT_FOUND, "record not found");
    CHECKC( info.creator == owner, info_err::PERMISSION_DENIED, "only the creator can operate" );
    CHECKC( info.status == info_status::RUNNING, info_err::NOT_AVAILABLE, "under maintenance" );
    CHECKC( has_auth(info.creator), info_err::PERMISSION_DENIED, "permission denied" );
}

const mdaoinfo::conf_t& mdaoinfo::_conf() {
    if (!_conf_ptr) {
        _conf_tbl_ptr = make_unique<conf_table_t>(MDAO_CONF, MDAO_CONF.value);
        CHECKC(_conf_tbl_ptr->exists(), info_err::SYSTEM_ERROR, "conf table not existed in contract" );
        _conf_ptr = make_unique<conf_t>(_conf_tbl_ptr->get());
    }
    return *_conf_ptr;
}

const mdaoinfo::conf_t2& mdaoinfo::_conf2() {
    if (!_conf_ptr2) {
        _conf_tbl_ptr2 = make_unique<conf_table_t2>(MDAO_CONF, MDAO_CONF.value);
        CHECKC(_conf_tbl_ptr2->exists(), info_err::SYSTEM_ERROR, "conf table2 not existed in contract" );
        _conf_ptr2 = make_unique<conf_t2>(_conf_tbl_ptr2->get());
    }
    return *_conf_ptr2;
}