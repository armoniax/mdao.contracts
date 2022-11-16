#include <mdao.stg/mdao.stg.hpp>
#include <thirdparty/utils.hpp>
#include "mdao.gov/mdao.gov.hpp"
#include <mdao.info/mdao.info.db.hpp>
#include <mdao.stake/mdao.stake.db.hpp>
#include <set>

#define VOTE_CREATE(bank, dao_code, creator, desc, title, vote_strategy_id, proposal_strategy_id) \
{ action(permission_level{get_self(), "active"_n }, bank, "create"_n, std::make_tuple( dao_code, creator, desc, title, vote_strategy_id, proposal_strategy_id)).send(); }

#define VOTE_EXCUTE(bank, owner, proposeid) \
{ action(permission_level{get_self(), "active"_n }, bank, "create"_n, std::make_tuple( owner, proposeid)).send(); }

ACTION mdaogov::create(const name& dao_code, const uint64_t& propose_strategy_id, 
                            const uint64_t& vote_strategy_id, const uint32_t& require_participation, 
                            const uint32_t& require_pass, const uint16_t& update_interval,
                            const uint16_t& voting_period)
{    
    auto conf = _conf();
    CHECKC( conf.status != conf_status::PENDING, gov_err::NOT_AVAILABLE, "under maintenance" );

    governance_t governance(dao_code);
    CHECKC( !_db.get(governance), gov_err::CODE_REPEAT, "governance already existing!" );

    dao_info_t::idx_t info_tbl(MDAO_INFO, MDAO_INFO.value);
    const auto info = info_tbl.find(dao_code.value);
    CHECKC( info != info_tbl.end(), gov_err::NOT_AVAILABLE, "dao not exists");
    CHECKC( has_auth(info->creator), gov_err::PERMISSION_DENIED, "only the creator can operate");

    strategy_t::idx_t stg(MDAO_STG, MDAO_STG.value);
    auto vote_strategy = stg.find(vote_strategy_id);
    auto propose_strategy = stg.find(propose_strategy_id);
    CHECKC( vote_strategy!= stg.end(), gov_err::STRATEGY_NOT_FOUND, "strategy not found");
    CHECKC( propose_strategy != stg.end(), gov_err::STRATEGY_NOT_FOUND, "strategy not found");

    switch (vote_strategy->type.value)
    {
        case strategy_type::NFT_PARENT_STAKE.value:
        case strategy_type::NFT_STAKE.value:
        case strategy_type::TOKEN_STAKE.value:{
            CHECKC( require_participation <= TEN_THOUSAND, gov_err::STRATEGY_NOT_FOUND, 
                        "participation no more than" + to_string(TEN_THOUSAND));
        }
        default:
            break;
    }

    governance.dao_code                                                = dao_code;
    governance.strategies[strategy_action_type::PROPOSAL]              = propose_strategy_id;
    governance.strategies[strategy_action_type::VOTE]                  = vote_strategy_id;
    governance.require_pass                                            = require_pass;
    governance.require_participation                                   = require_participation;
    governance.update_interval                                         = update_interval;
    governance.voting_period                                           = voting_period;
    _db.set(governance, _self);
}

ACTION mdaogov::setvotestg(const name& dao_code, const uint64_t& vote_strategy_id, 
                            const uint32_t& require_participation, const uint32_t& require_pass )
{

    auto conf = _conf();
    CHECKC( conf.status != conf_status::PENDING, gov_err::NOT_AVAILABLE, "under maintenance" );

    governance_t governance(dao_code);
    CHECKC( _db.get(governance), gov_err::RECORD_NOT_FOUND, "governance not exist!" );
    CHECKC( (governance.updated_at + (governance.update_interval * seconds_per_hour)) <= current_time_point(), gov_err::NOT_MODIFY, "cannot be modified for now" );

    dao_info_t::idx_t info_tbl(MDAO_INFO, MDAO_INFO.value);
    const auto info = info_tbl.find(dao_code.value);
    mdaogov:: _check_auth(governance, conf, *info);

    strategy_t::idx_t stg(MDAO_STG, MDAO_STG.value);
    auto vote_strategy = stg.find(vote_strategy_id);
    CHECKC( vote_strategy != stg.end(), gov_err::STRATEGY_NOT_FOUND, "strategy not found" );
    CHECKC( require_participation <= TEN_THOUSAND, gov_err::PARAM_ERROR, 
                "participation no more than" + to_string(TEN_THOUSAND));

    governance.updated_at                                                 = current_time_point();
    governance.strategies[strategy_action_type::VOTE]                     = vote_strategy_id;
    governance.require_pass                                               = require_pass;
    governance.require_participation                                      = require_participation;

    _db.set(governance, _self);
}

ACTION mdaogov::setproposestg(const name& dao_code, const uint64_t& propose_strategy_id)
{
    auto conf = _conf();
    CHECKC( conf.status != conf_status::PENDING, gov_err::NOT_AVAILABLE, "under maintenance" );
    require_auth( conf.managers[manager_type::PROPOSAL] );

    governance_t governance(dao_code);
    CHECKC( _db.get(governance), gov_err::RECORD_NOT_FOUND, "governance not exist" );
    CHECKC( (governance.updated_at + (governance.update_interval * seconds_per_hour)) <= current_time_point(), gov_err::NOT_MODIFY, "cannot be modified for now" );

    strategy_t::idx_t stg(MDAO_STG, MDAO_STG.value);
    CHECKC(stg.find(propose_strategy_id) != stg.end(), gov_err::STRATEGY_NOT_FOUND, "strategy not found");

    governance.strategies[strategy_action_type::PROPOSAL] = propose_strategy_id;
    governance.updated_at                                 = current_time_point();
    _db.set(governance, _self);
}

ACTION mdaogov::setlocktime(const name& dao_code, const uint16_t& update_interval)
{
    auto conf = _conf();
    CHECKC( conf.status != conf_status::PENDING, gov_err::NOT_AVAILABLE, "under maintenance" );
    require_auth( conf.managers[manager_type::PROPOSAL] );

    governance_t governance(dao_code);
    CHECKC( _db.get(governance), gov_err::RECORD_NOT_FOUND, "governance not exist" );
    CHECKC( update_interval >= governance.voting_period , gov_err::TIME_LESS_THAN_ZERO, "lock time less than vote time" );
    CHECKC( (governance.updated_at + (governance.update_interval * seconds_per_hour)) <= current_time_point(), gov_err::NOT_MODIFY, "cannot be modified for now" );

    governance.update_interval = update_interval;
    governance.updated_at = current_time_point();
    _db.set(governance, _self);
}

ACTION mdaogov::setvotetime(const name& dao_code, const uint16_t& voting_period)
{
    auto conf = _conf();
    CHECKC( conf.status != conf_status::PENDING, gov_err::NOT_AVAILABLE, "under maintenance" );
    require_auth( conf.managers[manager_type::PROPOSAL] );

    governance_t governance(dao_code);
    CHECKC( _db.get(governance), gov_err::RECORD_NOT_FOUND, "governance not exist" );
    CHECKC( voting_period <= governance.update_interval , gov_err::TIME_LESS_THAN_ZERO, "lock time less than vote time" );
    CHECKC( (governance.updated_at + (governance.update_interval * seconds_per_hour)) <= current_time_point(), gov_err::NOT_MODIFY, "cannot be modified for now" );

    governance.voting_period = voting_period;
    governance.updated_at = current_time_point();
    _db.set(governance, _self);
}
 
// ACTION mdaogov::startpropose(const name& creator, const name& dao_code, const string& title, const string& desc)
// {
//     require_auth( creator );
//     auto conf = _conf();
//     CHECKC( conf.status != conf_status::PENDING, gov_err::NOT_AVAILABLE, "under maintenance" );

//     governance_t governance(dao_code);
//     CHECKC( _db.get(governance) ,gov_err::RECORD_NOT_FOUND, "governance not exist" );
//     uint64_t vote_strategy_id = governance.strategies.at(strategy_action_type::VOTE);
//     uint64_t proposal_strategy_id = governance.strategies.at(strategy_action_type::PROPOSAL);

//     strategy_t::idx_t stg(MDAO_STG, MDAO_STG.value);
//     auto propose_strategy = stg.find(proposal_strategy_id);
//     CHECKC( propose_strategy != stg.end(), gov_err::STRATEGY_NOT_FOUND, "strategy not found" );
//     // CHECKC( false, gov_err::STRATEGY_NOT_FOUND, "strategy not found" );

//     int64_t stg_weight = 0;
//     _cal_votes(dao_code, *propose_strategy, creator, stg_weight);
//     CHECKC( stg_weight > 0, gov_err::INSUFFICIENT_WEIGHT, "insufficient strategy weight")

//     VOTE_CREATE(MDAO_PROPOSAL, dao_code, creator, desc, title, vote_strategy_id, proposal_strategy_id)
// }

void mdaogov::deletegov(name dao_code) {
    governance_t governance(dao_code);

    _db.del(governance);
}

ACTION mdaogov::setpropmodel(const name& dao_code, const name& propose_model)
{
    auto conf = _conf();
    CHECKC( conf.status != conf_status::PENDING, gov_err::NOT_AVAILABLE, "under maintenance" );
    require_auth( conf.managers[manager_type::PROPOSAL] );
    CHECKC( propose_model == propose_model_type::MIX || propose_model == propose_model_type::PROPOSAL, gov_err::PARAM_ERROR, "param error" );

    governance_t governance(dao_code);
    CHECKC( _db.get(governance), gov_err::RECORD_NOT_FOUND, "governance not exist" );
    CHECKC( (governance.updated_at + (governance.update_interval * seconds_per_hour)) <= current_time_point(), gov_err::NOT_MODIFY, "cannot be modified for now" );

    governance.proposal_model   = propose_model;
    governance.updated_at  = current_time_point();
    _db.set(governance, _self);
}

void mdaogov::_cal_votes(const name dao_code, const strategy_t& vote_strategy, const name voter, int64_t& value) {
    switch(vote_strategy.type.value){
        case strategy_type::TOKEN_STAKE.value : 
        case strategy_type::NFT_STAKE.value : 
        case strategy_type::NFT_PARENT_STAKE.value:{
            value = mdao::strategy::cal_stake_weight(MDAO_STG, vote_strategy.id, dao_code, MDAO_STAKE, voter);
            break;
        }
        case strategy_type::TOKEN_BALANCE.value:
        case strategy_type::NFT_BALANCE.value:
        case strategy_type::NFT_PARENT_BALANCE.value:
        case strategy_type::TOKEN_SUM.value: {
            value = mdao::strategy::cal_balance_weight(MDAO_STG, vote_strategy.id, voter);
            break;
         }
        default : {
           CHECKC( false, gov_err::TYPE_ERROR, "type error" );
        }
    }
}

const mdaogov::conf_t& mdaogov::_conf() {
    if (!_conf_ptr) {
        _conf_tbl_ptr = make_unique<conf_table_t>(MDAO_CONF, MDAO_CONF.value);
        CHECKC(_conf_tbl_ptr->exists(), gov_err::SYSTEM_ERROR, "conf table not existed in contract" );
        _conf_ptr = make_unique<conf_t>(_conf_tbl_ptr->get());
    }
    return *_conf_ptr;
}

void mdaogov::_check_auth(const governance_t& governance, const conf_t& conf, const dao_info_t& info) {
    if(governance.proposal_model == propose_model_type::MIX ){
        CHECKC(has_auth(conf.managers.at(manager_type::PROPOSAL)) || has_auth(info.creator), gov_err::NOT_MODIFY, "cannot be modified for now" );
    }else{
        CHECKC( has_auth(conf.managers.at(manager_type::PROPOSAL)), gov_err::NOT_MODIFY, "cannot be modified for now" );
    }   
}
