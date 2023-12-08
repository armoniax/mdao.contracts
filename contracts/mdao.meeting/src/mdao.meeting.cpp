#include <mdao.meeting/mdao.meeting.hpp>
#include <thirdparty/utils.hpp>
#include <thirdparty/wasm_db.hpp>
#include <amax.token/amax.token.hpp>
#include <mdao.info/mdao.info.db.hpp>
using namespace wasm;


static constexpr name ACTIVE_PERM       = "active"_n;
#define TRANSFER_OUT(token_contract, to, quantity, memo)                             \
            token::transfer_action(token_contract, {{get_self(), ACTIVE_PERM}}) \
               .send(get_self(), to, quantity, memo);

void mdaomeeting::init( const name& admin, const asset& fee ){
    require_auth(_self);
    CHECKC( is_account( admin ), err::RECORD_NOT_FOUND, "admin account not exists" );

    _gstate.admin = admin;
    _gstate.fee.quantity = fee;
}

void mdaomeeting::setreceiver( const name& receiver){
    require_auth(_self);
    CHECKC( is_account( receiver ), err::RECORD_NOT_FOUND, "receiver account not exists" );
    _gstate.receiver = receiver;
}

void mdaomeeting::setsplit(const uint64_t& split_id){
    CHECKC( has_auth(_self) || has_auth(_gstate.admin), err::NO_AUTH,"no auth")

    _gstate.split_id = split_id;
}
// memo1 dao:$dao.code:$group_id:$month 
void mdaomeeting::ontransfer(const name& from, const name& to, const asset& quant, const string& memo){

    if (from == get_self() || to != get_self()) return;

    CHECKC( quant.amount > 0 , err::PARAM_ERROR,"Quantity error")
    CHECKC( _gstate.running, err::NO_AUTH,"paused")
    CHECKC( from != to, err::PARAM_ERROR, "cannot send to self" )

    vector<string_view> memo_params = split(memo, ":");

    if (memo_params.size() == 0){
            return;
    }

    auto action_name = name(memo_params[0]);
    switch (action_name.value){
            case meeting_action_name::DAO:{
                name dao_code = name(memo_params[1]);
                string group_id = string(memo_params[2]);
                int month  = stoi( string( memo_params[3] ));
                CHECKC( month > 0,err::PARAM_ERROR,"month musdt be > 0")
                _create_renew_dao( from, dao_code, group_id, quant,month);
            }
                break;
            default:

                CHECKC( false, err::PARAM_ERROR, "memo not Supported");
                break;
        }
    
}

void mdaomeeting::setwhitelist(const set<name>& accounts){
    CHECKC( has_auth(_self) || has_auth(_gstate.admin), err::NO_AUTH,"no auth")
    for ( name n : accounts ){
        auto itr = _whitelist_tbl.find( n.value );
        db::set( _whitelist_tbl, itr, _self, [&]( auto& p, bool is_new ){
            p.dao_code = n;
        });
    }
}

void mdaomeeting::delwhitelist(const set<name>& accounts){
    
    CHECKC( has_auth(_self) || has_auth(_gstate.admin), err::NO_AUTH,"no auth")

    for ( name n : accounts ){

        auto itr = _whitelist_tbl.find( n.value );
        if ( itr != _whitelist_tbl.end() )
            _whitelist_tbl.erase(itr);
    }
}   


void mdaomeeting::_create_renew_dao(const name& from, const name& dao_code, const string& group_id, const asset& quantity, const uint64_t& month){

    auto whitelist_itr = _whitelist_tbl.find( dao_code.value );
    CHECKC( whitelist_itr !=  _whitelist_tbl.end(), err::NO_AUTH,"Not yet open")

    dao_info_t::idx_t dao_info(MDAO_INFO_ACCOUNT,MDAO_INFO_ACCOUNT.value);
    auto dao_itr = dao_info.find( dao_code.value );
    CHECKC( dao_itr != dao_info.end(),err::PARAM_ERROR,"dao not found")
    // CHECKC( dao_itr -> creator == from, err::NO_AUTH,"not creator")
    
    name token_contract = get_first_receiver();
    CHECKC( token_contract == _gstate.fee.contract,err::PARAM_ERROR,"token contract must be " + _gstate.fee.contract.to_string())

    asset need_quantity = _gstate.fee.quantity * month;
    CHECKC( need_quantity <= quantity, err::PARAM_ERROR,"Insufficient payment quantity")

    asset refund_quantity = quantity - need_quantity;
    auto itr = _meeting_tbl.find(dao_code.value);
    
    auto now = current_time_point();
    db::set( _meeting_tbl, itr, _self, [&]( auto& p, bool is_new ){

        if ( is_new ){
            p.created_at = now;
            p.dao_code = dao_code;
           
            p.expired_at = now;
        }
        p.creator = from;
        p.group_id = group_id;
        if ( p.expired_at < now )
            p.expired_at = now;
        // p.expired_at = p.expired_at < now ? now : p.expired_at;
        p.status = status::ENABLED;
        p.expired_at += month * seconds_per_month;
        p.updated_at = now;
        
    });

    if (refund_quantity.amount > 0)
        TRANSFER_OUT(token_contract, from, refund_quantity,"refund")

    if ( _gstate.split_id > 0 )
        TRANSFER_OUT(token_contract, SPLIT_ACCOUNT, need_quantity,"plan:" + to_string(_gstate.split_id))
    else if( is_account( _gstate.receiver )){
        TRANSFER_OUT(token_contract, _gstate.receiver, need_quantity,"meeting:" + to_string(_gstate.split_id))
    }
}