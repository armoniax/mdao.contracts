#include "mdao.stg/mdao.stg.hpp"
#include "thirdparty/utils.hpp"

using namespace mdao;
using namespace picomath;

void strategy::create( const name& creator,
            const uint64_t& stg_id,
            const string& stg_name,
            const string& stg_algo,
            const name& type,
            const name& ref_contract,
            const refsymbol& ref_sym){
    require_auth(creator);
    
    strategy_t strategy(stg_id);
    CHECKC( !_db.get(strategy), err::RECORD_FOUND, "stg is exists")
    CHECKC( stg_name.size() < MAX_CONTENT_SIZE, err::OVERSIZED, "stg_name length should less than "+ to_string(MAX_CONTENT_SIZE) )
    CHECKC( stg_algo.size() < MAX_ALGO_SIZE, err::OVERSIZED, "stg_algo length should less than "+ to_string(MAX_ALGO_SIZE) )
    CHECKC( type == strategy_type::TOKEN_BALANCE ||
            type == strategy_type::TOKEN_STAKE ||
            type == strategy_type::TOKEN_SUM ||
            type == strategy_type::NFT_BALANCE ||
            type == strategy_type::NFT_STAKE ||
            type == strategy_type::NFT_PARENT_STAKE ||
            type == strategy_type::NFT_PARENT_BALANCE, stg_err::PARAM_ERROR, "type error" )
    _check_contract_and_sym(ref_contract, ref_sym, type);

    strategy.creator        = creator;
    strategy.stg_name       = stg_name;
    strategy.stg_algo       = stg_algo;
    strategy.type           = type;
    strategy.ref_sym        = ref_sym;
    strategy.ref_contract   = ref_contract;
    strategy.status         = strategy_status::published;
    strategy.created_at     = current_time_point();

    _db.set( strategy, creator );
}


void strategy::thresholdstg(const name& creator,
                const string& stg_name,
                const uint64_t& balance_value,
                const name& type,
                const name& ref_contract,
                const refsymbol& ref_sym){
    require_auth(creator);

    CHECKC( stg_name.size() < MAX_CONTENT_SIZE+1, stg_err::OVERSIZED, "stg_name length should less than "+ to_string(MAX_CONTENT_SIZE) )

    CHECKC( type == strategy_type::TOKEN_BALANCE ||
            type == strategy_type::TOKEN_STAKE ||
            type == strategy_type::TOKEN_SUM ||
            type == strategy_type::NFT_BALANCE ||
            type == strategy_type::NFT_STAKE ||
            type == strategy_type::NFT_PARENT_STAKE ||
            type == strategy_type::NFT_PARENT_BALANCE, stg_err::PARAM_ERROR, "type error" )
    _check_contract_and_sym(ref_contract, ref_sym, type);
    string stg_algo = "min(x-"+ to_string(balance_value) + ",1)";
                                               
    auto strategies         = strategy_t::idx_t(_self, _self.value);
    auto pid                = _gstate.last_stg_id++;
    auto strategy           = strategy_t(pid);
    strategy.id             = pid;
    strategy.creator        = creator;
    strategy.stg_name       = stg_name;
    strategy.stg_algo       = stg_algo;
    strategy.type           = type;
    strategy.ref_sym        = ref_sym;
    strategy.ref_contract   = ref_contract;
    strategy.status         = strategy_status::published;
    strategy.created_at     = current_time_point();

    _db.set( strategy, creator );
}

void strategy::balancestg(const name& creator,
                const string& stg_name,
                const uint64_t& weight_value,
                const name& type,
                const name& ref_contract,
                const refsymbol& ref_sym){
    require_auth(creator);

    CHECKC( stg_name.size() < MAX_CONTENT_SIZE+1, stg_err::OVERSIZED, "stg_name length should less than "+ to_string(MAX_CONTENT_SIZE) )
    CHECKC( weight_value != 0, stg_err::PARAM_ERROR, "balance_value cannot equal zero" )

    CHECKC( type == strategy_type::TOKEN_BALANCE ||
            type == strategy_type::TOKEN_STAKE ||
            type == strategy_type::TOKEN_SUM ||
            type == strategy_type::NFT_BALANCE ||
            type == strategy_type::NFT_STAKE ||
            type == strategy_type::NFT_PARENT_STAKE ||
            type == strategy_type::NFT_PARENT_BALANCE, stg_err::PARAM_ERROR, "type error" )
    _check_contract_and_sym(ref_contract, ref_sym, type);

    string stg_algo = "x*"+ to_string(weight_value);

    auto strategies         = strategy_t::idx_t(_self, _self.value);
    auto pid                = _gstate.last_stg_id++;
    auto strategy           = strategy_t(pid);
    strategy.id             = pid;
    strategy.creator        = creator;
    strategy.stg_name       = stg_name;
    strategy.stg_algo       = stg_algo;
    strategy.type           = type;
    strategy.ref_sym        = ref_sym;
    strategy.ref_contract   = ref_contract;
    strategy.status         = strategy_status::published;
    strategy.created_at     = current_time_point();

    _db.set( strategy, creator );
}

void strategy::setalgo( const name& creator,
                        const uint64_t& stg_id,
                        const string& stg_algo,
                        const name& type,
                        const name& ref_contract,
                        const refsymbol& ref_sym ){
    require_auth( creator );
    
    CHECKC( type == strategy_type::TOKEN_BALANCE ||
            type == strategy_type::TOKEN_STAKE ||
            type == strategy_type::TOKEN_SUM ||
            type == strategy_type::NFT_BALANCE ||
            type == strategy_type::NFT_STAKE ||
            type == strategy_type::NFT_PARENT_STAKE ||
            type == strategy_type::NFT_PARENT_BALANCE, stg_err::PARAM_ERROR, "type error" )
    _check_contract_and_sym(ref_contract, ref_sym, type);

    strategy_t stg = strategy_t(stg_id);
    CHECKC( _db.get( stg ), stg_err::RECORD_NOT_FOUND, "strategy not found: " + to_string(stg_id))
    CHECKC( stg.creator == creator, stg_err::NO_AUTH, "require creator auth")

    stg.stg_algo    = stg_algo;
    stg.status      = strategy_status::published;
    _db.set( stg, creator );
}

void strategy::verify( const name& creator,
                const uint64_t& stg_id,
                const uint64_t& value,
                const uint64_t& expect_weight ){
    require_auth( creator );

    CHECKC( expect_weight > 0, stg_err::NOT_POSITIVE, "require positive weight to verify stg" )

    strategy_t stg = strategy_t( stg_id );
    CHECKC( _db.get( stg ), stg_err::RECORD_NOT_FOUND, "strategy not found: " + to_string( stg_id ) )
    CHECKC( stg.status != strategy_status::published, stg_err::NO_AUTH, "cannot verify published strategy" )
    CHECKC( stg.creator == creator, stg_err::NO_AUTH, "require creator auth" )

    int128_t weight = cal_weight( get_self(), value, stg_id);
    // CHECKC( weight == expect_weight, stg_err::UNRESPECT_RESULT, "algo result weight" )

    stg.status = strategy_status::verified;
    _db.set( stg, creator );
}

void strategy::testalgo( const name& account, const uint64_t& stg_id ){
    strategy_t stg = strategy_t( stg_id );
    CHECKC( _db.get( stg ), stg_err::RECORD_NOT_FOUND, "strategy not found: " + to_string( stg_id ) )
    
    weight_struct weight_str = cal_balance_weight(get_self(), stg_id, account);
    // CHECKC(false, stg_err::NONE, "weight: "+ to_string(weight_str.weight));
}

void strategy::remove( const name& creator,
                       const uint64_t& stg_id ){
    require_auth( creator );

    strategy_t stg = strategy_t( stg_id );
    CHECKC( _db.get( stg ), stg_err::RECORD_NOT_FOUND, "strategy not found: " + to_string(stg_id) )
    CHECKC( stg.creator == creator, stg_err::NO_AUTH, "require creator auth" )
    CHECKC( stg.status != strategy_status::published, stg_err::NO_AUTH, "cannot remove published strategy" );

    _db.del( stg );
}

void strategy::publish( const name& creator,
                        const uint64_t& stg_id ){
    require_auth( creator );

    strategy_t stg = strategy_t( stg_id );
    CHECKC( _db.get( stg ), stg_err::RECORD_NOT_FOUND, "strategy not found: " + to_string( stg_id ) )
    CHECKC( stg.creator == creator, stg_err::NO_AUTH, "require creator auth" )
    CHECKC( stg.status == strategy_status::verified, stg_err::NO_AUTH, "please verify your strategy before publish" );

    stg.status = strategy_status::published;
    _db.set( stg, creator );
}

void strategy::_check_contract_and_sym( const name& contract,
                                            const refsymbol& ref_symbol,
                                            const name& type){
    int64_t value = 0;
    switch (type.value)
    {
        case strategy_type::NFT_STAKE.value:
        case strategy_type::NFT_BALANCE.value:
        case strategy_type::NFT_PARENT_STAKE.value:
        case strategy_type::NFT_PARENT_BALANCE.value:{
            nsymbol sym = std::get<nsymbol>(ref_symbol);
            value = amax::ntoken::get_supply(contract, sym);
            break;
        }
        case strategy_type::TOKEN_BALANCE.value:
        case strategy_type::TOKEN_SUM.value:
        case strategy_type::TOKEN_STAKE.value: {
            symbol sym = std::get<symbol>(ref_symbol);
            value = aplink::token::get_supply(contract, sym.code()).amount;
            break;
        }
        default:
            check(false, "unsupport calculating type");
            break;
    }

    CHECKC( value > 0, stg_err::PARAM_ERROR, "contract or symbol error " )
}