#include "xdao.stg/xdao.stg.hpp"
#include "thirdparty/utils.hpp"

using namespace xdao;
using namespace picomath;

void strategy::create( const name& creator, 
            const name& type, 
            const string& stg_name, 
            const string& stg_algo,
            const asset& require_apl,
            const symbol_code& require_symbol_code,
            const name& ref_contract ){
    require_auth(creator);

    CHECKC( require_apl.symbol == APL_SYMBOL && require_apl.amount >= 0, err::SYMBOL_MISMATCH, "only support non-negtive APL" )
    CHECKC( stg_name.size() < MAX_CONTENT_SIZE, err::OVERSIZED, "stg_name length should less than "+ to_string(MAX_CONTENT_SIZE) )
    CHECKC( stg_algo.size() < MAX_ALGO_SIZE, err::OVERSIZED, "stg_algo length should less than "+ to_string(MAX_ALGO_SIZE) )

    auto strategies         = strategy_t::idx_t(_self, _self.value);
    auto pid                = strategies.available_primary_key();
    auto strategy           = strategy_t(pid);
    strategy.id             = pid;
    strategy.creator        = creator;
    strategy.type           = type;
    strategy.stg_name       = stg_name;
    strategy.stg_algo       = stg_algo;
    strategy.require_apl    = require_apl;
    strategy.status         = strategy_status::testing;
    strategy.created_at     = current_time_point();


    if(type != strategy_type::unlimited){
        CHECKC( is_account( ref_contract ), err::ACCOUNT_INVALID, "Contract account invalid" )
        CHECKC( token::get_supply( ref_contract, require_symbol_code).amount>0, err::SYMBOL_MISMATCH, "cannot find valid token in "+ref_contract.to_string() );
        strategy.ref_contract           = ref_contract;
        strategy.require_symbol_code    = require_symbol_code;
    }

    _db.set( strategy, creator );
}

void strategy::setalgo( const name& creator, 
                        const uint64_t& stg_id, 
                        const string& stg_algo ){
    require_auth( creator );

    strategy_t stg = strategy_t(stg_id);
    CHECKC( _db.get( stg ), err::RECORD_NOT_FOUND, "strategy not found: " + to_string(stg_id))
    CHECKC( stg.creator == creator, err::NO_AUTH, "require creator auth")
    CHECKC( stg.status != strategy_status::published, err::NO_AUTH, "cannot monidfy published strategy");

    stg.stg_algo    = stg_algo;
    stg.status      = strategy_status::testing;
    _db.set( stg, creator );
}

void strategy::verify( const name& creator,
                const uint64_t& stg_id, 
                const name& account,
                const uint64_t& respect_weight ){
    require_auth( creator );

    CHECKC( is_account(account), err::ACCOUNT_INVALID, "account invalid" )
    CHECKC( respect_weight > 0, err::NOT_POSITIVE, "require positive weight to verify stg" )

    strategy_t stg = strategy_t( stg_id );
    CHECKC( _db.get( stg ), err::RECORD_NOT_FOUND, "strategy not found: " + to_string( stg_id ) )
    CHECKC( stg.status != strategy_status::published, err::NO_AUTH, "cannot verify published strategy" )

    int32_t weight = _cal_stg_weight( stg, account );
    CHECKC( weight == respect_weight, err::UNRESPECT_RESULT, "algo result weight is: "+to_string(weight) )

    stg.status = strategy_status::tested;
    _db.set( stg, creator );
}

void strategy::testalgo( const name& account, const string& algo, const double& param ){
    require_auth( account );

    PicoMath pm;
    auto &x = pm.addVariable( "x" );
    x = param;
    auto result = pm.evalExpression( algo.c_str() );
    if (result.isOk()) {
        double r = result.getResult();
        check(false, "result: "+ to_string(r));
    }
    check(false, result.getError());
}

void strategy::remove( const name& creator, 
                       const uint64_t& stg_id ){
    require_auth( creator );

    strategy_t stg = strategy_t( stg_id );
    CHECKC( _db.get( stg ), err::RECORD_NOT_FOUND, "strategy not found: " + to_string(stg_id) )
    CHECKC( stg.creator == creator, err::NO_AUTH, "require creator auth" )
    CHECKC( stg.status != strategy_status::published, err::NO_AUTH, "cannot remove published strategy" );

    _db.del( stg );
}

void strategy::publish( const name& creator, 
                        const uint64_t& stg_id ){
    require_auth( creator );

    strategy_t stg = strategy_t( stg_id );
    CHECKC( _db.get( stg ), err::RECORD_NOT_FOUND, "strategy not found: " + to_string( stg_id ) )
    CHECKC( stg.creator == creator, err::NO_AUTH, "require creator auth" )
    CHECKC( stg.status == strategy_status::tested, err::NO_AUTH, "please verify your strategy before publish" );

    stg.status = strategy_status::published;
    _db.set( stg, creator );
}
