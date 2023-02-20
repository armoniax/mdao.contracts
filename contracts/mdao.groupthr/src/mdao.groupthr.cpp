#include <mdao.groupthr/mdao.groupthr.hpp>

#define TOKEN_TRANSFER(bank, to, quantity, memo) \
{ action(permission_level{get_self(), "active"_n }, bank, "transfer"_n, std::make_tuple( _self, to, quantity, memo )).send(); }


void mdaogroupthr::ontransfer()
{

    auto contract = get_first_receiver();
    if (token_contracts.count(contract) > 0) {
        execute_function(&mdaogroupthr::_on_token_transfer);

    } else if (ntoken_contracts.count(contract)>0) {
        execute_function(&mdaogroupthr::_on_ntoken_transfer);
    }
}

void mdaogroupthr::addbybalance( const name &mermber, const uint64_t &groupthr_id )
{
    require_auth(mermber);

    groupthr_t groupthr(groupthr_id);
    CHECKC( _db.get(groupthr), err::RECORD_NOT_FOUND, "group threshold config not exists" );
    CHECKC( groupthr.expired_time >= current_time_point(), groupthr_err::ALREADY_EXPIRED, "group threshold expired" );
    CHECKC( groupthr.type == threshold_type::TOKEN_BALANCE || groupthr.type == threshold_type::NFT_BALANCE, err::PARAM_ERROR, "group threshold type error" );
    if (groupthr.type == threshold_type::TOKEN_BALANCE) {
        extended_asset threshold = std::get<extended_asset>(groupthr.threshold);
        asset balance = aplink::token::get_balance(threshold.contract, mermber, threshold.quantity.symbol.code());
        CHECKC( threshold.quantity <= balance, groupthr_err::QUANTITY_NOT_ENOUGH, "quantity not enough" );

        _create_mermber(mermber, groupthr.id);
    } else {
        extended_nasset threshold = std::get<extended_nasset>(groupthr.threshold);
        nasset balance = amax::ntoken::get_balance(threshold.contract, mermber, threshold.quantity.symbol);
        CHECKC( threshold.quantity <= balance, groupthr_err::QUANTITY_NOT_ENOUGH, "quantity not enough" );

        _create_mermber(mermber, groupthr.id);
    }
}

void mdaogroupthr::setthreshold( const uint64_t &groupthr_id, const refasset &threshold, const name &type )
{

    groupthr_t groupthr(groupthr_id);
    CHECKC( _db.get(groupthr), err::RECORD_NOT_FOUND, "group threshold config not exists" );
    CHECKC( groupthr.expired_time >= current_time_point(), groupthr_err::ALREADY_EXPIRED, "group threshold expired" );

    dao_info_t::idx_t info_tbl(_self, _self.value);
    const auto info = info_tbl.find(groupthr.dao_code.value);
    CHECKC( info != info_tbl.end(), err::RECORD_NOT_FOUND, "mdao not found" );
    CHECKC( has_auth(info->creator), groupthr_err::PERMISSION_DENIED, "permission denied" );

    if ( groupthr.type == threshold_type::TOKEN_BALANCE || groupthr.type == threshold_type::TOKEN_PAY) {
        extended_asset asset_threshold = std::get<extended_asset>(threshold);
        CHECKC( asset_threshold.quantity.amount > 0, groupthr_err::NOT_POSITIVE, "threshold can not be negative" );
    } else if(groupthr.type == threshold_type::NFT_BALANCE || groupthr.type == threshold_type::NFT_PAY) {
        extended_nasset nasset_threshold = std::get<extended_nasset>(groupthr.threshold);
        CHECKC( nasset_threshold.quantity.amount > 0, groupthr_err::NOT_POSITIVE, "threshold can not be negative" );
    }else{
        CHECKC( false, err::PARAM_ERROR, "param error" );
    }

    groupthr.type = type;
    groupthr.threshold = threshold;
    _db.set(groupthr, _self);
}

void mdaogroupthr::delmermbers( vector<uint64_t> &mermbers )
{
    CHECKC( mermbers.size() > 0, err::PARAM_ERROR, "param error" );

    auto conf = _conf();
    CHECKC( conf.status != conf_status::PENDING, groupthr_err::NOT_AVAILABLE, "under maintenance" );
    CHECKC( has_auth(conf.admin), groupthr_err::PERMISSION_DENIED, "only the admin can operate" );

    mermber_t mermber;
    vector<uint64_t>::iterator mermber_iter;
    for( mermber_iter = mermbers.begin(); mermber_iter != mermbers.end(); mermber_iter++ ){
        mermber.id = *mermber_iter;
        bool is_exists = _db.get(mermber);
        if(!is_exists) continue;
        _db.del(mermber);
    }

}

//memo format : 'target : asset : group_id : dao_code : type :  contract'
//memo format : 'target : id : pid : amount : group_id : dao_code : type : contract'
//memo format : 'target : groupthr_id'
void mdaogroupthr::_on_token_transfer( const name &from,
                                        const name &to,
                                        const asset &quantity,
                                        const string &memo)
{

    if (from == _self || to != _self) return;

    auto parts = split( memo, ":" );

    if ( parts.size() == 6 && parts[0] == "createbytoken" ) {

        auto asset_threshold      = asset_from_string(parts[1]);
        CHECKC( asset_threshold.amount > 0, err::PARAM_ERROR, "threshold amount not positive" );

        auto group_id   = parts[2];
        auto dao_code   = name(parts[3]);
        auto type       = name(parts[4]);
        CHECKC( type == threshold_type::TOKEN_BALANCE || type == threshold_type::TOKEN_PAY, err::PARAM_ERROR, "type error" );

        auto contract   = name(parts[5]);
        extended_asset threshold = extended_asset(asset_threshold, contract);
        int64_t value = amax::token::get_supply(contract, asset_threshold.symbol.code()).amount;
        CHECKC( value > 0, groupthr_err::SYMBOL_MISMATCH, "symbol mismatch" );

        _create_groupthr(dao_code, from, group_id, threshold, type);
    }else if (parts.size() == 8 && parts[0] == "createbyntoken") {

        auto id         = to_uint64(parts[1], "id parse uint error");
        auto parent_id  = to_uint64(parts[2], "parent_id parse uint error");
        auto amount     = to_int64(parts[3], "quantity parse int error");
        CHECKC( amount > 0, err::PARAM_ERROR, "threshold amount not positive" );

        auto group_id   = parts[4];
        auto dao_code   = name(parts[5]);
        auto type       = name(parts[6]);
        CHECKC( type == threshold_type::NFT_BALANCE || type == threshold_type::NFT_PAY, err::PARAM_ERROR, "type error" );

        auto contract   = name(parts[7]);
        nsymbol nsym(id, parent_id);
        nasset nft_quantity(amount, nsym);
        extended_nasset threshold(nft_quantity, contract);

        int64_t value = amax::ntoken::get_supply(contract, nsym);
        CHECKC( value > 0, groupthr_err::SYMBOL_MISMATCH, "symbol mismatch" );

        _create_groupthr(dao_code, from, group_id, threshold, type);
    } else if (parts.size() == 2 && parts[0] == "enter") {

        auto groupthr_id  = to_uint64(parts[1], "groupthr_id parse uint error");
        groupthr_t groupthr(groupthr_id);
        CHECKC( _db.get(groupthr), err::RECORD_NOT_FOUND, "group threshold config not exists" );
        CHECKC( groupthr.expired_time >= current_time_point(), groupthr_err::ALREADY_EXPIRED, "group threshold expired" );

        extended_asset threshold = std::get<extended_asset>(groupthr.threshold);
        CHECKC( threshold.quantity <= quantity, groupthr_err::QUANTITY_NOT_ENOUGH, "quantity not enough" );

        _create_mermber(from, groupthr.id);
    } else {
        CHECKC( false, err::PARAM_ERROR, "param error" );
    }
}

//memo format : 'target : groupthr_id'
void mdaogroupthr::_on_ntoken_transfer( const name& from,
                                        const name& to,
                                        const std::vector<nasset>& assets,
                                        const string& memo )
{

    if (from == _self || to != _self) return;

    auto parts = split( memo, ":" );

    if (parts.size() == 2 && parts[0] == "enter") {
        auto groupthr_id  = to_uint64(parts[1], "groupthr_id parse uint error");

        groupthr_t groupthr(groupthr_id);
        CHECKC( _db.get(groupthr), err::RECORD_NOT_FOUND, "group threshold config not exists" );
        CHECKC( groupthr.expired_time >= current_time_point(), groupthr_err::ALREADY_EXPIRED, "group threshold expired" );

        extended_nasset threshold = std::get<extended_nasset>(groupthr.threshold);
        nasset quantity = assets.at(0);
        CHECKC( threshold.quantity <= quantity, groupthr_err::QUANTITY_NOT_ENOUGH, "quantity not enough" );

        _create_mermber(from, groupthr.id);
    } else {
        CHECKC( false, err::PARAM_ERROR, "param error" );
    }
}

void mdaogroupthr::_create_groupthr( const name& dao_code,
                                        const name& from,
                                        const string_view& group_id,
                                        const refasset& threshold,
                                        const name& type )
{
    dao_info_t::idx_t info_tbl(_self, _self.value);
    const auto info = info_tbl.find(dao_code.value);
    CHECKC( info != info_tbl.end(), err::RECORD_NOT_FOUND, "mdao not found" );
    CHECKC( info->creator == from, groupthr_err::PERMISSION_DENIED, "permission denied" );


    groupthr_t::idx_t groupthr_tbl(_self, _self.value);
    auto groupthr_index = groupthr_tbl.get_index<"bygroupid"_n>();
    auto groupthr_itr = groupthr_index.find(HASH256(string(group_id)));

    bool is_exists = groupthr_itr != groupthr_index.end();

    groupthr_t groupthr;
     if (is_exists && groupthr_itr->expired_time >= current_time_point()) {
        groupthr.expired_time = groupthr_itr->expired_time + (seconds_per_day * 30);
    } else {
        groupthr.expired_time = time_point_sec(current_time_point()) + (seconds_per_day * 30);
    }
    groupthr.dao_code       = dao_code;
    groupthr.group_id       = group_id;
    groupthr.threshold      = threshold;
    groupthr.type           = type;

    _db.set(groupthr, _self);
}
//今天几号

void mdaogroupthr::_create_mermber( const name& from,
                                    const uint64_t& groupthr_id )
{

    mermber_t::idx_t mermber_tbl(_self, _self.value);
    auto mermber_index = mermber_tbl.get_index<"byidgroupid"_n>();
    uint128_t sec_index = (uint128_t)from.value << 64 | (uint128_t)groupthr_id;
    auto mermber_itr = mermber_index.find(sec_index);

    bool is_exists = mermber_itr != mermber_index.end();
    mermber_t mermber;
    if (is_exists && mermber_itr->expired_time >= current_time_point()) {
        mermber.expired_time = mermber_itr->expired_time + (seconds_per_day * 30);
    } else {
        mermber.expired_time = time_point_sec(current_time_point()) + (seconds_per_day * 30);
    }
    mermber.groupthr_id    = groupthr_id;
    mermber.mermber        = from;
    _db.set(mermber, _self);
}

const mdaogroupthr::conf_t& mdaogroupthr::_conf() {
    if (!_conf_ptr) {
        _conf_tbl_ptr = make_unique<conf_table_t>(MDAO_CONF, MDAO_CONF.value);
        CHECKC(_conf_tbl_ptr->exists(), groupthr_err::SYSTEM_ERROR, "conf table not existed in contract" );
        _conf_ptr = make_unique<conf_t>(_conf_tbl_ptr->get());
    }
    return *_conf_ptr;
}
