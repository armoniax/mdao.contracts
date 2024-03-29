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
                            const uint64_t& vote_strategy_id, const int128_t& require_pass, 
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
    CHECKC( vote_strategy!= stg.end(), gov_err::STRATEGY_NOT_FOUND, "vote strategy not found");
    CHECKC( propose_strategy != stg.end(), gov_err::STRATEGY_NOT_FOUND, "propose strategy not found");
    CHECKC( vote_strategy->status == strategy_status::published && propose_strategy->status == strategy_status::published, 
            gov_err::STRATEGY_STATUS_ERROR, "strategy type must be published" );
    CHECKC( voting_period >= 3 && voting_period <= 14, gov_err::PARAM_ERROR, "voting_period no less than 3 and no more than 14");
    CHECKC( require_pass > 0 , gov_err::PARAM_ERROR, "require_pass no less than 0" );

    governance.dao_code                                                = dao_code;
    governance.strategies[strategy_action_type::PROPOSAL]              = propose_strategy_id;
    governance.strategies[strategy_action_type::VOTE]                  = vote_strategy_id;
    governance.require_pass                                            = require_pass;
    governance.voting_period                                           = voting_period;
    governance.updated_at                                              = current_time_point();
    _db.set(governance, info->creator);
}

ACTION mdaogov::setvotestg(const name& dao_code, const uint64_t& vote_strategy_id)
{

    auto conf = _conf();
    CHECKC( conf.status != conf_status::PENDING, gov_err::NOT_AVAILABLE, "under maintenance" );

    governance_t governance(dao_code);
    CHECKC( _db.get(governance), gov_err::RECORD_NOT_FOUND, "governance not exist!" );
    
    dao_info_t::idx_t info_tbl(MDAO_INFO, MDAO_INFO.value);
    const auto info = info_tbl.find(dao_code.value);
    CHECKC(has_auth(info->creator), gov_err::NOT_MODIFY, "cannot be modified for now" );

    strategy_t::idx_t stg(MDAO_STG, MDAO_STG.value);
    auto vote_strategy = stg.find(vote_strategy_id);
    CHECKC( vote_strategy != stg.end(), gov_err::STRATEGY_NOT_FOUND, "strategy not found" );
    CHECKC( vote_strategy->status == strategy_status::published, 
            gov_err::STRATEGY_STATUS_ERROR, "strategy type must be published" );

    governance.updated_at                                                 = current_time_point();
    governance.strategies[strategy_action_type::VOTE]                     = vote_strategy_id;

    _db.set(governance, _self);
}

ACTION mdaogov::setproposestg(const name& dao_code, const uint64_t& propose_strategy_id)
{
    auto conf = _conf();
    CHECKC( conf.status != conf_status::PENDING, gov_err::NOT_AVAILABLE, "under maintenance" );

    dao_info_t::idx_t info_tbl(MDAO_INFO, MDAO_INFO.value);
    const auto info = info_tbl.find(dao_code.value);
    CHECKC(has_auth(info->creator), gov_err::NOT_MODIFY, "cannot be modified for now" );

    governance_t governance(dao_code);
    CHECKC( _db.get(governance), gov_err::RECORD_NOT_FOUND, "governance not exist" );

    strategy_t::idx_t stg(MDAO_STG, MDAO_STG.value);
    CHECKC(stg.find(propose_strategy_id) != stg.end(), gov_err::STRATEGY_NOT_FOUND, "strategy not found");

    governance.strategies[strategy_action_type::PROPOSAL] = propose_strategy_id;
    governance.updated_at                                 = current_time_point();
    _db.set(governance, _self);
}

ACTION mdaogov::setgov(const name& dao_code, const uint16_t& voting_period, 
                            const int128_t& require_pass,const uint64_t& propose_strategy_id,
                            const uint64_t& vote_strategy_id)
{
    auto conf = _conf();
    CHECKC( conf.status != conf_status::PENDING, gov_err::NOT_AVAILABLE, "under maintenance" );
    
    dao_info_t::idx_t info_tbl(MDAO_INFO, MDAO_INFO.value);
    const auto info = info_tbl.find(dao_code.value);
    CHECKC(has_auth(info->creator), gov_err::NOT_MODIFY, "cannot be modified for now" );

    governance_t governance(dao_code);
    CHECKC( _db.get(governance), gov_err::RECORD_NOT_FOUND, "governance not exist" );
    CHECKC( require_pass > 0 , gov_err::PARAM_ERROR, "require_pass no less than 0" );
    CHECKC( voting_period >= 3 && voting_period <= 14, gov_err::PARAM_ERROR, "require_pass no less than 3 and no more than 14");

    strategy_t::idx_t stg(MDAO_STG, MDAO_STG.value);
    auto vote_strategy = stg.find(vote_strategy_id);
    auto propose_strategy = stg.find(propose_strategy_id);
    CHECKC( vote_strategy!= stg.end(), gov_err::STRATEGY_NOT_FOUND, "vote strategy not found");
    CHECKC( propose_strategy != stg.end(), gov_err::STRATEGY_NOT_FOUND, "propose strategy not found");
    CHECKC( vote_strategy->status == strategy_status::published && propose_strategy->status == strategy_status::published, 
            gov_err::STRATEGY_STATUS_ERROR, "strategy type must be published" );
            
    governance.voting_period = voting_period;
    governance.require_pass  = require_pass;
    governance.updated_at = current_time_point();
    governance.strategies[strategy_action_type::PROPOSAL] = propose_strategy_id;
    governance.strategies[strategy_action_type::VOTE]     = vote_strategy_id;

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

ACTION mdaogov::delgov()
{
    governance_t::idx_t governance(_self, _self.value);
    auto governance_itr = governance.begin(); 
    for(;governance_itr != governance.end();){
        governance_itr = governance.erase(governance_itr);
    }
}

// void mdaogov::_cal_votes(const name dao_code, const strategy_t& vote_strategy, const name voter, int64_t& value) {
//     switch(vote_strategy.type.value){
//         case strategy_type::TOKEN_STAKE.value : 
//         case strategy_type::NFT_STAKE.value : 
//         case strategy_type::NFT_PARENT_STAKE.value:{
//             value = mdao::strategy::cal_stake_weight(vote_strategy, dao_code, MDAO_STAKE, voter).weight;
//             break;
//         }
//         case strategy_type::TOKEN_BALANCE.value:
//         case strategy_type::NFT_BALANCE.value:
//         case strategy_type::NFT_PARENT_BALANCE.value:
//         case strategy_type::TOKEN_SUM.value: {
//             value = mdao::strategy::cal_balance_weight(vote_strategy, voter, 0).weight;
//             break;
//          }
//         default : {
//            CHECKC( false, gov_err::TYPE_ERROR, "type error" );
//         }
//     }
// }

const mdaogov::conf_t& mdaogov::_conf() {
    if (!_conf_ptr) {
        _conf_tbl_ptr = make_unique<conf_table_t>(MDAO_CONF, MDAO_CONF.value);
        CHECKC(_conf_tbl_ptr->exists(), gov_err::SYSTEM_ERROR, "conf table not existed in contract" );
        _conf_ptr = make_unique<conf_t>(_conf_tbl_ptr->get());
    }
    return *_conf_ptr;
}

