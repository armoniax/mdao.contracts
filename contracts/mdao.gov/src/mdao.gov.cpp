#include <mdao.stg/mdao.stg.hpp>
#include <thirdparty/utils.hpp>
#include "mdao.gov/mdao.gov.hpp"
#include <mdao.info/mdao.info.db.hpp>
#include <set>

#define VOTE_CREATE(dao_code, creator, name, desc, title, vote_strategy_id, type) \
{ action(permission_level{get_self(), "active"_n }, "mdao.propose"_n, "create"_n, std::make_tuple( dao_code, creator, name, desc, title, vote_strategy_id, type)).send(); }

#define VOTE_EXCUTE(owner, proposeid) \
{ action(permission_level{get_self(), "active"_n }, "mdao.propose"_n, "create"_n, std::make_tuple( owner, proposeid)).send(); }

ACTION mdaogov::create(const name& dao_code, const name& propose_strategy_code, 
                            const name& vote_strategy_code, const uint64_t& propose_strategy_id, 
                            const uint64_t& vote_strategy_id, const uint32_t& require_participation, 
                            const uint32_t& require_pass )
{
    auto conf = _conf();
    CHECKC( conf.status != conf_status::PENDING, gov_err::NOT_AVAILABLE, "under maintenance" );
    require_auth( conf.managers[manager_type::GOV] );

    governance_t governance(dao_code);
    CHECKC( !_db.get(governance), gov_err::CODE_REPEAT, "governance already existing!" );

    strategy_t::idx_t stg(MDAO_STG, MDAO_STG.value);
    auto vote_strategy = stg.find(vote_strategy_id);
    auto propose_strategy = stg.find(propose_strategy_id);
    CHECKC( vote_strategy!= stg.end(), gov_err::STRATEGY_NOT_FOUND, "strategy not found");
    CHECKC( propose_strategy != stg.end(), gov_err::STRATEGY_NOT_FOUND, "strategy not found");
    CHECKC( (vote_strategy->type != strategy_type::nftstaking && vote_strategy->type != strategy_type::tokenstaking) || 
            ((vote_strategy->type == strategy_type::nftstaking || vote_strategy->type == strategy_type::tokenstaking) && require_participation <= 10000 && require_pass <= 10000), 
            gov_err::STRATEGY_NOT_FOUND, "strategy not found");

    governance.dao_code                                     = dao_code;
    governance.propose_strategy[propose_strategy_code]      = propose_strategy_id;
    governance.vote_strategy[vote_strategy_code]            = vote_strategy_id;
    governance.require_pass[vote_strategy_id]               = require_pass;
    governance.require_participation[vote_strategy_id]      = require_participation;

    _db.set(governance, _self);
}

ACTION mdaogov::setvotestg(const name& dao_code, const name& vote_strategy_code, 
                            const uint64_t& vote_strategy_id, const uint32_t& require_participation, 
                            const uint32_t& require_pass )
{
    auto conf = _conf();
    CHECKC( conf.status != conf_status::PENDING, gov_err::NOT_AVAILABLE, "under maintenance" );
    require_auth( conf.managers[manager_type::PROPOSAL] );

    governance_t governance(dao_code);
    CHECKC( !_db.get(governance), gov_err::CODE_REPEAT, "governance already existing!" );
    CHECKC( (governance.last_updated_at + (governance.limit_update_hours * 3600)) <= current_time_point(), gov_err::NOT_MODIFY, "cannot be modified for now" );

    strategy_t::idx_t stg(MDAO_STG, MDAO_STG.value);
    auto vote_strategy = stg.find(vote_strategy_id);
    CHECKC( vote_strategy != stg.end(), gov_err::STRATEGY_NOT_FOUND, "strategy not found" );
    CHECKC( (vote_strategy->type != strategy_type::nftstaking && vote_strategy->type != strategy_type::tokenstaking) || 
            ((vote_strategy->type == strategy_type::nftstaking || vote_strategy->type == strategy_type::tokenstaking) && require_participation <= 100000 && require_pass <= 100000), 
            gov_err::PARAM_ERROR, "param error");

    governance.last_updated_at                           = current_time_point();
    governance.vote_strategy[vote_strategy_code]         = vote_strategy_id;
    governance.require_pass[vote_strategy_id]            = require_pass;
    governance.require_participation[vote_strategy_id]   = require_participation;

    _db.set(governance, _self);
}

ACTION mdaogov::setproposestg(const name& dao_code, const name& propose_strategy_code, const uint64_t& propose_strategy_id)
{
    auto conf = _conf();
    CHECKC( conf.status != conf_status::PENDING, gov_err::NOT_AVAILABLE, "under maintenance" );
    require_auth( conf.managers[manager_type::PROPOSAL] );

    governance_t governance(dao_code);
    CHECKC( _db.get(governance), gov_err::RECORD_NOT_FOUND, "governance not exist" );
    CHECKC( (governance.last_updated_at + (governance.limit_update_hours * 3600)) <= current_time_point(), gov_err::NOT_MODIFY, "cannot be modified for now" );

    strategy_t::idx_t stg(MDAO_STG, MDAO_STG.value);
    CHECKC(stg.find(propose_strategy_id) != stg.end(), gov_err::STRATEGY_NOT_FOUND, "strategy not found");

    governance.propose_strategy[propose_strategy_code]     = propose_strategy_id;
    governance.last_updated_at                             = current_time_point();
    _db.set(governance, _self);
}

// ACTION mdaogov::setleastvote(const name& dao_code, const uint32_t& require_least_votes)
// {
//     auto conf = _conf();

//     CHECKC( conf.status != conf_status::PENDING, gov_err::NOT_AVAILABLE, "under maintenance" );
//     require_auth( conf.managers[manager_type::PROPOSAL] );

//     governance_t governance(dao_code);
//     CHECKC( !_db.get(governance), gov_err::CODE_REPEAT, "governance already existing!" );
//     CHECKC( (governance.last_updated_at + (governance.limit_update_hours * 3600)) <= current_time_point(), gov_err::NOT_MODIFY, "cannot be modified for now" );

//     governance.require_least_votes = require_least_votes;
//     governance.last_updated_at = current_time_point();
//     _db.set(governance, _self);
// }

ACTION mdaogov::setlocktime(const name& dao_code, const uint16_t& limit_update_hours)
{
    auto conf = _conf();
    CHECKC( conf.status != conf_status::PENDING, gov_err::NOT_AVAILABLE, "under maintenance" );
    require_auth( conf.managers[manager_type::PROPOSAL] );

    governance_t governance(dao_code);
    CHECKC( !_db.get(governance), gov_err::CODE_REPEAT, "governance already existing!" );
    CHECKC( limit_update_hours >= governance.voting_limit_hours , gov_err::TIME_LESS_THAN_ZERO, "lock time less than vote time" );
    CHECKC( (governance.last_updated_at + (governance.limit_update_hours * 3600)) <= current_time_point(), gov_err::NOT_MODIFY, "cannot be modified for now" );

    governance.limit_update_hours = limit_update_hours;
    governance.last_updated_at = current_time_point();
    _db.set(governance, _self);
}

ACTION mdaogov::setvotetime(const name& dao_code, const uint16_t& voting_limit_hours)
{
    auto conf = _conf();
    CHECKC( conf.status != conf_status::PENDING, gov_err::NOT_AVAILABLE, "under maintenance" );
    require_auth( conf.managers[manager_type::PROPOSAL] );

    governance_t governance(dao_code);
    CHECKC( !_db.get(governance), gov_err::CODE_REPEAT, "governance already existing!" );
    CHECKC( voting_limit_hours <= governance.limit_update_hours , gov_err::TIME_LESS_THAN_ZERO, "lock time less than vote time" );
    CHECKC( (governance.last_updated_at + (governance.limit_update_hours * 3600)) <= current_time_point(), gov_err::NOT_MODIFY, "cannot be modified for now" );

    governance.voting_limit_hours = voting_limit_hours;
    governance.last_updated_at = current_time_point();
    _db.set(governance, _self);
}

ACTION mdaogov::startpropose(const name& creator, const name& dao_code, const string& title,
                                 const string& proposal_name, const string& desc, 
                                 const name& plan_type, const name& propose_strategy_code, 
                                 const name& vote_strategy_code)
{
    require_auth( creator );
    auto conf = _conf();
    CHECKC( conf.status != conf_status::PENDING, gov_err::NOT_AVAILABLE, "under maintenance" );

    CHECKC( plan_type == plan_type::SINGLE || plan_type == plan_type::MULTIPLE, gov_err::TYPE_ERROR, "type error" );

    dao_info_t::idx_t info_tbl(MDAO_INFO, MDAO_INFO.value);
    const auto info = info_tbl.find(dao_code.value);
    CHECKC( info->creator == creator, gov_err::PERMISSION_DENIED, "only the creator can operate");

    governance_t governance(dao_code);
    CHECKC( _db.get(governance) ,gov_err::RECORD_NOT_FOUND, "record not found" );
    
    strategy_t::idx_t stg(MDAO_STG, MDAO_STG.value);
    auto propose_strategy = stg.find(governance.propose_strategy.at(propose_strategy_code));
    CHECKC( propose_strategy != stg.end(), gov_err::STRATEGY_NOT_FOUND, "strategy not found" );

    int64_t value;
    if((propose_strategy->type != strategy_type::nftstaking && propose_strategy->type != strategy_type::tokenstaking)){
        accounts accountstable(propose_strategy->ref_contract, creator.value);
        const auto ac = accountstable.find(propose_strategy->require_symbol_code.raw()); 
        value = ac->balance.amount;
    }
    int32_t stg_weight = mdao::strategy::cal_weight(MDAO_STG, value, creator, governance.propose_strategy.at(propose_strategy_code) );
    CHECKC( stg_weight > 0, gov_err::INSUFFICIENT_WEIGHT, "insufficient strategy weight")

    VOTE_CREATE(dao_code, creator, proposal_name, desc, title, governance.vote_strategy.at(vote_strategy_code), plan_type)

}

const mdaogov::conf_t& mdaogov::_conf() {
    if (!_conf_ptr) {
        _conf_tbl_ptr = make_unique<conf_table_t>(MDAO_CONF, MDAO_CONF.value);
        CHECKC(_conf_tbl_ptr->exists(), gov_err::SYSTEM_ERROR, "conf table not existed in contract" );
        _conf_ptr = make_unique<conf_t>(_conf_tbl_ptr->get());
    }
    return *_conf_ptr;
}
