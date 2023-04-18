#include <mdao.groupthr/mdao.groupthr.hpp>

#define TOKEN_TRANSFER(bank, to, quantity, memo) \
{ action(permission_level{get_self(), "active"_n }, bank, "transfer"_n, std::make_tuple( _self, to, quantity, memo )).send(); }

#define NTOKEN_TRANSFER(bank, to, quantity, memo) \
{ action(permission_level{get_self(), "active"_n }, bank, "transfer"_n, std::make_tuple( _self, to, quantity, memo )).send(); }

#define EXTEND_PLAN(is_exists_and_unexpired, expired, current, number) \
{ is_exists_and_unexpired ? expired + seconds_per_month * number : current + seconds_per_month * number }


void mdaogroupthr::setglobal( asset crt_groupthr_fee, asset join_member_fee)
{
    require_auth(_self);
    asset crt_groupthr_fee_supply = token::get_supply(AMAX_CONTRACT, crt_groupthr_fee.symbol.code());
    asset join_member_fee_supply = token::get_supply(AMAX_CONTRACT, join_member_fee.symbol.code());

    CHECKC( crt_groupthr_fee.amount >= 0, err::NOT_POSITIVE, "crt_groupthr_fee must be positive" );
    CHECKC( join_member_fee.amount >= 0, err::NOT_POSITIVE, "join_member_fee must be positive" );
    CHECKC( crt_groupthr_fee_supply.amount > 0, err::SYMBOL_MISMATCH, "symbol mismatch" );
    CHECKC( crt_groupthr_fee_supply.amount > 0, err::SYMBOL_MISMATCH, "symbol mismatch" );

    _gstate.crt_groupthr_fee = crt_groupthr_fee;
    _gstate.join_member_fee = join_member_fee;
}

void mdaogroupthr::ontransfer()
{

    auto contract = get_first_receiver();
    if (token_contracts.count(contract) > 0) {
        execute_function(&mdaogroupthr::_on_token_transfer);

    } else if (ntoken_contracts.count(contract)>0) {
        execute_function(&mdaogroupthr::_on_ntoken_transfer);
    }
}

void mdaogroupthr::join( const name &member, const uint64_t &groupthr_id )
{
    require_auth(member);

    groupthr_t groupthr(groupthr_id);
    CHECKC( _db.get(groupthr), err::RECORD_NOT_FOUND, "group threshold config not exists" );
    CHECKC( groupthr.expired_time >= current_time_point(), groupthr_err::ALREADY_EXPIRED, "group threshold expired" );
    CHECKC( groupthr.threshold_type == threshold_type::TOKEN_BALANCE || groupthr.threshold_type == threshold_type::NFT_BALANCE, err::PARAM_ERROR, "group threshold type error" );
    CHECKC( groupthr.enable_threshold, groupthr_err::CLOSED, "group threshold is closed" );
    if (groupthr.threshold_type == threshold_type::TOKEN_BALANCE) {
        extended_asset threshold = std::get<extended_asset>(groupthr.threshold_plan.at(threshold_plan_type::MONTH));
        asset balance = amax::token::get_balance(threshold.contract, member, threshold.quantity.symbol.code());
        CHECKC( threshold.quantity <= balance, groupthr_err::QUANTITY_NOT_ENOUGH, "quantity not enough" );

        _join_balance_member(member, groupthr_id, groupthr.threshold_type);
    } else if(groupthr.threshold_type == threshold_type::NFT_BALANCE) {
        extended_nasset threshold = std::get<extended_nasset>(groupthr.threshold_plan.at(threshold_plan_type::MONTH));
        nasset balance = amax::ntoken::get_balance(threshold.contract, member, threshold.quantity.symbol);
        CHECKC( threshold.quantity <= balance, groupthr_err::QUANTITY_NOT_ENOUGH, "quantity not enough" );

        _join_balance_member(member, groupthr_id, groupthr.threshold_type);
    } else {
        CHECKC( false, err::PARAM_ERROR, "param error" );
    }
}

void mdaogroupthr::setthreshold(const uint64_t &groupthr_id, const refasset &threshold, const name &plan_type)
{

    groupthr_t groupthr(groupthr_id);
    CHECKC( _db.get(groupthr), err::RECORD_NOT_FOUND, "group threshold config not exists" );
    CHECKC( groupthr.expired_time >= current_time_point(), groupthr_err::ALREADY_EXPIRED, "group threshold expired" );
    CHECKC( has_auth(groupthr.owner), groupthr_err::PERMISSION_DENIED, "only the owner can operate" );

    uint128_t plan_union_thr_type = GET_UNION_TYPE_ID(groupthr.threshold_type, plan_type);
    switch (plan_union_thr_type)
    {
        case plan_union_threshold_type::TOKEN_BALANCE_MONTH:
        case plan_union_threshold_type::TOKEN_PAY_MONTH:
        case plan_union_threshold_type::TOKEN_PAY_QUARTER:
        case plan_union_threshold_type::TOKEN_PAY_YEAR:{
            extended_asset asset_threshold = std::get<extended_asset>(threshold);
            CHECKC( asset_threshold.quantity.amount > 0, groupthr_err::NOT_POSITIVE, "threshold can not be negative" );
            break;
        }
        case plan_union_threshold_type::NFT_BALANCE_MONTH:
        case plan_union_threshold_type::NFT_PAY_MONTH:
        case plan_union_threshold_type::NFT_PAY_QUARTER:
        case plan_union_threshold_type::NFT_PAY_YEAR:{
            extended_nasset nasset_threshold = std::get<extended_nasset>(threshold);
            CHECKC( nasset_threshold.quantity.amount > 0, groupthr_err::NOT_POSITIVE, "threshold can not be negative" );
            break;
        }
        default:
            CHECKC( false, err::PARAM_ERROR, "threshold plan type code error");
    }

    groupthr.threshold_plan[plan_type] = threshold;
    _db.set(groupthr, _self);
}

void mdaogroupthr::enablegthr( const uint64_t &groupthr_id, const bool &enable_threshold)
{
    groupthr_t groupthr(groupthr_id);
    CHECKC( _db.get(groupthr), err::RECORD_NOT_FOUND, "group threshold config not exists" );
    CHECKC( groupthr.expired_time >= current_time_point(), groupthr_err::ALREADY_EXPIRED, "group threshold expired" );
    CHECKC( has_auth(groupthr.owner), groupthr_err::PERMISSION_DENIED, "only the owner can operate" );

    groupthr.enable_threshold = enable_threshold;
    _db.set(groupthr, _self);
}

void mdaogroupthr::delgroupthr( const uint64_t &groupthr_id)
{
    groupthr_t groupthr(groupthr_id);
    CHECKC( _db.get(groupthr), err::RECORD_NOT_FOUND, "group threshold config not exists" );
    // CHECKC( has_auth(groupthr.owner), groupthr_err::PERMISSION_DENIED, "only the owner can operate" );

    _db.del(groupthr);
}

void mdaogroupthr::delmembers(vector<deleted_member> &deleted_members)
{
    CHECKC( deleted_members.size() > 0, err::PARAM_ERROR, "param error" );

    auto conf = _conf();
    CHECKC( conf.status != conf_status::PENDING, groupthr_err::NOT_AVAILABLE, "under maintenance" );
    CHECKC( has_auth(conf.admin), groupthr_err::PERMISSION_DENIED, "only the admin can operate" );

    member_t member;
    vector<deleted_member>::iterator member_iter;
    for( member_iter = deleted_members.begin(); member_iter != deleted_members.end(); member_iter++ ){

        member_t::idx_t member_tbl(_self, _self.value);
        auto member_index = member_tbl.get_index<"byidgroupid"_n>();
        uint128_t sec_index = (uint128_t)(*member_iter).account.value << 64 | (uint128_t)(*member_iter).groupthr_id;
        auto member_itr = member_index.find(sec_index);
        if( member_itr == member_index.end() ) continue;
        _db.del(*member_itr);
    }
}
  
  /**
  * @brief transfer token to this contract
  *
  * @param from
  * @param to
  * @param quantity
  * @param memo: five formats:
  *       1) createbytoken : $type : $asset : $contract : $group_id : $plan_type                        -- create group threshold by token
  *       2) createbyntoken : $type : $id : $pid : $amount : $contract : $group_id : $plan_type         -- create group threshold by ntoken
  *       3) renewgroupthr : $group_id                                                                  -- group threshold renewal    
  *       4) joinfee : $groupthr_id                                                                     -- transfer to join the service charge                                             
  *       5) join : $groupthr_id : plan_type                                                            -- join member                                               
  *
  */
void mdaogroupthr::_on_token_transfer( const name &from,
                                        const name &to,
                                        const asset &quantity,
                                        const string &memo)
{

    if (from == _self || to != _self) return;

    auto parts = split( memo, ":" );

    if ( parts.size() == 6 && parts[0] == "createbytoken" ) {

        int64_t months          = quantity / _gstate.crt_groupthr_fee;
        CHECKC( months > 0, err::PARAM_ERROR, "param error" );

        auto type               = name(parts[1]);
        auto asset_threshold    = asset_from_string(parts[2]);
        CHECKC( asset_threshold.amount > 0, err::PARAM_ERROR, "threshold amount not positive" );

        auto contract           = name(parts[3]);
        auto group_id           = parts[4];
        CHECKC( type == threshold_type::TOKEN_BALANCE || type == threshold_type::TOKEN_PAY, err::PARAM_ERROR, "type error" );

        extended_asset threshold = extended_asset(asset_threshold, contract);
        int64_t value = amax::token::get_supply(contract, asset_threshold.symbol.code()).amount;
        CHECKC( value > 0, groupthr_err::SYMBOL_MISMATCH, "symbol mismatch" );
        
        auto plan_type           = name(parts[5]);
        CHECKC( plan_type == threshold_plan_type::MONTH || 
                plan_type == threshold_plan_type::QUARTER || 
                plan_type == threshold_plan_type::YEAR, 
                err::PARAM_ERROR, "param error" );

        _create_groupthr(from, group_id, threshold, type, months, plan_type);
        
    } else if (parts.size() == 8 && parts[0] == "createbyntoken") {

        int64_t months  = quantity / _gstate.crt_groupthr_fee;
        CHECKC( months > 0, err::PARAM_ERROR, "param error" );

        auto type       = name(parts[1]);
        auto id         = to_uint64(parts[2], "id parse uint error");
        auto parent_id  = to_uint64(parts[3], "parent_id parse uint error");
        auto amount     = to_int64(parts[4], "quantity parse int error");
        CHECKC( amount > 0, err::PARAM_ERROR, "threshold amount not positive" );

        auto contract   = name(parts[5]);
        auto group_id   = parts[6];
        CHECKC( type == threshold_type::NFT_BALANCE || type == threshold_type::NFT_PAY, err::PARAM_ERROR, "type error" );

        nsymbol nsym(id, parent_id);
        int64_t value = amax::ntoken::get_supply(contract, nsym);
        CHECKC( value > 0, groupthr_err::SYMBOL_MISMATCH, "symbol mismatch" );
        
        nasset nft_quantity(amount, nsym);
        extended_nasset threshold(nft_quantity, contract);
        
        auto plan_type           = name(parts[7]);
        CHECKC( plan_type == threshold_plan_type::MONTH || 
                plan_type == threshold_plan_type::QUARTER || 
                plan_type == threshold_plan_type::YEAR, 
                err::PARAM_ERROR, "param error" );
                
        _create_groupthr(from, group_id, threshold, type, months, plan_type);
        
    } else if ( parts.size() == 2 && parts[0] == "renewgroupthr" ) {
      
        int64_t months          = quantity / _gstate.crt_groupthr_fee;
        CHECKC( months > 0, err::PARAM_ERROR, "param error" );
        
        auto group_id           = parts[1];
        _renewal_groupthr(group_id, months);
        
    } else if (parts.size() == 2 && parts[0] == "joinfee") {
      
        auto groupthr_id  = to_uint64(parts[1], "groupthr_id parse uint error");
        groupthr_t groupthr(groupthr_id);
        CHECKC( _db.get(groupthr), err::RECORD_NOT_FOUND, "group threshold config not exists" );
        CHECKC( groupthr.expired_time >= current_time_point(), groupthr_err::ALREADY_EXPIRED, "group threshold expired" );
        CHECKC(groupthr.threshold_type == threshold_type::TOKEN_PAY || groupthr.threshold_type == threshold_type::NFT_PAY, groupthr_err::TYPE_ERROR, "group threshold type mismatch");
        CHECKC(quantity >= _gstate.join_member_fee, err::FEE_INSUFFICIENT, "fee insufficient");

        _init_member(from, groupthr_id);
        
    } else if (parts.size() == 3 && parts[0] == "join") {

        auto groupthr_id  = to_uint64(parts[1], "groupthr_id parse uint error");
        auto plan_type   = name(parts[2]);

        groupthr_t groupthr(groupthr_id);
        CHECKC( _db.get(groupthr), err::RECORD_NOT_FOUND, "group threshold config not exists" );
        CHECKC( groupthr.expired_time >= current_time_point(), groupthr_err::ALREADY_EXPIRED, "group threshold expired" );
        CHECKC( groupthr.threshold_type == threshold_type::TOKEN_PAY, groupthr_err::TYPE_ERROR, "group threshold type mismatch" );
        CHECKC( groupthr.enable_threshold, groupthr_err::CLOSED, "group threshold is closed" );

        extended_asset extended_quantity= extended_asset(quantity, get_first_receiver());
        _join_expense_member(from, groupthr, plan_type, extended_quantity);

        TOKEN_TRANSFER( get_first_receiver(), groupthr.owner, quantity, string("join group threshold") );
        
    } else {
        CHECKC( false, err::PARAM_ERROR, "param error" );
    }
}

/**
* @brief transfer token to this contract
*
* @param from
* @param to
* @param quantity
* @param memo: one formats:
*       1) join : $groupthr_id : $plan_type                     -- join member 
*/
void mdaogroupthr::_on_ntoken_transfer( const name& from,
                                        const name& to,
                                        const std::vector<nasset>& assets,
                                        const string& memo )
{
    if (from == _self || to != _self) return;

    auto parts = split( memo, ":" );

    if (parts.size() == 3 && parts[0] == "join") {
      
        auto groupthr_id  = to_uint64(parts[1], "groupthr_id parse uint error");
        auto plan_type   = name(parts[2]);

        groupthr_t groupthr(groupthr_id);
        CHECKC( _db.get(groupthr), err::RECORD_NOT_FOUND, "group threshold config not exists" );
        CHECKC( groupthr.expired_time >= current_time_point(), groupthr_err::ALREADY_EXPIRED, "group threshold expired" );
        CHECKC( groupthr.threshold_type == threshold_type::NFT_PAY, groupthr_err::TYPE_ERROR, "group threshold type mismatch" );
        CHECKC( groupthr.enable_threshold, groupthr_err::CLOSED, "group threshold is closed" );
        nasset quantity = assets.at(0);
        extended_nasset extended_quantity= extended_nasset(quantity, get_first_receiver());
        _join_expense_member(from, groupthr, plan_type, extended_quantity);
        
        NTOKEN_TRANSFER( get_first_receiver(), groupthr.owner, assets, string("join group threshold") );
        
    } else {
        CHECKC( false, err::PARAM_ERROR, "param error" );
    }
}

void mdaogroupthr::_create_groupthr(    const name& from,
                                        const string_view& group_id,
                                        const refasset& threshold,
                                        const name& type,
                                        const int64_t& months,
                                        const name& plan_type)
{
    groupthr_t::idx_t groupthr_tbl(_self, _self.value);
    auto groupthr_index = groupthr_tbl.get_index<"bygroupid"_n>();
    auto groupthr_itr = groupthr_index.find(HASH256(string(group_id)));
    CHECKC( groupthr_itr == groupthr_index.end(), err::RECORD_FOUND, "groupthr already exists" );

    auto gid = _gstate.last_groupthr_id++;
    groupthr_t groupthr(gid);
    groupthr.expired_time   = time_point_sec(current_time_point()) + months * seconds_per_month;
    groupthr.group_id       = group_id;
    groupthr.threshold_plan[plan_type] = threshold;
    groupthr.threshold_type = type;
    groupthr.owner          = from;
    _db.set(groupthr, _self);
}

void mdaogroupthr::_renewal_groupthr(   const string_view& group_id,
                                        const int64_t& months)
{
    groupthr_t::idx_t groupthr_tbl(_self, _self.value);
    auto groupthr_index = groupthr_tbl.get_index<"bygroupid"_n>();
    auto groupthr_itr = groupthr_index.find(HASH256(string(group_id)));
    CHECKC( groupthr_itr != groupthr_index.end(), err::RECORD_NOT_FOUND, "groupthr not found" );
    groupthr_t group = *groupthr_itr;
    bool unexpired = group.expired_time >= current_time_point();
    group.expired_time = EXTEND_PLAN( unexpired, group.expired_time, time_point_sec(current_time_point()), months );
    _db.set(group, _self);
}

void mdaogroupthr::_join_expense_member( const name& from,
                                    const groupthr_t& groupthr,
                                    const name& plan_tpye,
                                    const refasset& quantity)
{
    auto threshold = groupthr.threshold_plan.find(plan_tpye);
    CHECKC( threshold != groupthr.threshold_plan.end(), err::PARAM_ERROR, "param error" );
    CHECKC( groupthr.threshold_plan.at(plan_tpye) <= quantity, groupthr_err::QUANTITY_NOT_ENOUGH, "quantity not enough" );

    member_t::idx_t member_tbl(_self, _self.value);
    auto member_index = member_tbl.get_index<"byidgroupid"_n>();
    uint128_t sec_index = (uint128_t)from.value << 64 | (uint128_t)groupthr.id;
    auto member_itr = member_index.find(sec_index);

    bool is_exists           = member_itr != member_index.end();
    CHECKC( is_exists || (!is_exists && _gstate.join_member_fee.amount == 0) , groupthr_err::NOT_INITED, "please pay the handling charge " );
  
    member_t member          = is_exists ? *member_itr : member_t(_gstate.last_member_id++);
    bool unexpired           = member.expired_time >= current_time_point();

    switch (plan_tpye.value)
    {
        case threshold_plan_type::MONTH.value:{
            member.expired_time = EXTEND_PLAN( is_exists && unexpired, member.expired_time, time_point_sec(current_time_point()), month );
            break;
        }
        case threshold_plan_type::QUARTER.value:{
            member.expired_time = EXTEND_PLAN( is_exists && unexpired, member.expired_time, time_point_sec(current_time_point()), months_per_quarter );
            break;
        }
        case threshold_plan_type::YEAR.value:{
            member.expired_time = EXTEND_PLAN( is_exists && unexpired, member.expired_time, time_point_sec(current_time_point()), months_per_year );
            break;
        }
        default:
            CHECKC( false, err::PARAM_ERROR, "threshold plan type code error");
    }
    
    if(!is_exists){
        member.groupthr_id      = groupthr.id;
        member.member           = from;
        member.status           = member_status::CREATED; 
        member.type             = groupthr.threshold_type; 
    }

    _db.set(member, _self);
}

void mdaogroupthr::_join_balance_member( const name& from,
                                        const uint64_t& groupthr_id,
                                        const name& threshold_type)
{
    member_t::idx_t member_tbl(_self, _self.value);
    auto member_index = member_tbl.get_index<"byidgroupid"_n>();
    uint128_t sec_index = (uint128_t)from.value << 64 | (uint128_t)groupthr_id;
    auto member_itr = member_index.find(sec_index);
    CHECKC( member_itr == member_index.end(), err::RECORD_FOUND, "record is exists" );

    auto mid = _gstate.last_member_id++;
    member_t member(mid);
    member.groupthr_id      = groupthr_id;
    member.member           = from;
    member.status           = member_status::CREATED;
    member.type             = threshold_type; 

    _db.set(member, _self);
}

void mdaogroupthr::_init_member( const name& from,
                                const uint64_t& groupthr_id)
{
    member_t::idx_t member_tbl(_self, _self.value);
    auto member_index = member_tbl.get_index<"byidgroupid"_n>();
    uint128_t sec_index = (uint128_t)from.value << 64 | (uint128_t)groupthr_id;
    auto member_itr = member_index.find(sec_index);
    CHECKC( member_itr == member_index.end(), err::RECORD_FOUND, "record is exists" );

    auto mid = _gstate.last_member_id++;
    member_t member(mid);
    member.groupthr_id      = groupthr_id;
    member.member           = from;
    member.status           = member_status::INIT;
    _db.set(member, _self);
}

const mdaogroupthr::conf_t& mdaogroupthr::_conf() {
    if (!_conf_ptr) {
        _conf_tbl_ptr = make_unique<conf_table_t>(MDAO_CONF, MDAO_CONF.value);
        CHECKC(_conf_tbl_ptr->exists(), groupthr_err::SYSTEM_ERROR, "conf table not existed in contract" );
        _conf_ptr = make_unique<conf_t>(_conf_tbl_ptr->get());
    }
    return *_conf_ptr;
}