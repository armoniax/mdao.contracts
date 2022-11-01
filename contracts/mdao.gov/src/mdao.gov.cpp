#include <mdao.stg/mdao.stg.hpp>
#include <thirdparty/utils.hpp>
#include "mdao.gov/mdao.gov.hpp"
#include <mdao.info/mdao.info.db.hpp>
#include <mdao.stake/mdao.stake.db.hpp>
#include <set>

#define VOTE_CREATE(bank, dao_code, creator, name, desc, title, vote_strategy_id, proposal_strategy_id, type) \
{ action(permission_level{get_self(), "active"_n }, bank, "create"_n, std::make_tuple( dao_code, creator, name, desc, title, vote_strategy_id, proposal_strategy_id, type)).send(); }

#define VOTE_EXCUTE(bank, owner, proposeid) \
{ action(permission_level{get_self(), "active"_n }, bank, "create"_n, std::make_tuple( owner, proposeid)).send(); }

ACTION mdaogov::create(const name& dao_code, const uint64_t& propose_strategy_id, 
                            const uint64_t& vote_strategy_id, const uint32_t& require_participation, 
                            const uint32_t& require_pass )
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
    CHECKC( (vote_strategy->type != strategy_type::nftstake && vote_strategy->type != strategy_type::tokenstake) || 
            ((vote_strategy->type == strategy_type::nftstake || vote_strategy->type == strategy_type::tokenstake) && require_participation <= TEN_THOUSAND && require_pass <= TEN_THOUSAND), 
            gov_err::STRATEGY_NOT_FOUND, "strategy not found");

    governance.dao_code                                                = dao_code;
    governance.strategys[strategy_action_type::PROPOSAL]               = propose_strategy_id;
    governance.strategys[strategy_action_type::VOTE]                   = vote_strategy_id;
    governance.require_pass[strategy_action_type::VOTE.value]          = require_pass;
    governance.require_participation[strategy_action_type::VOTE.value] = require_participation;

    _db.set(governance, _self);
}

ACTION mdaogov::setvotestg(const name& dao_code, const uint64_t& vote_strategy_id, 
                            const uint32_t& require_participation, const uint32_t& require_pass )
{
    auto conf = _conf();
    CHECKC( conf.status != conf_status::PENDING, gov_err::NOT_AVAILABLE, "under maintenance" );
    require_auth( conf.managers[manager_type::PROPOSAL] );

    governance_t governance(dao_code);
    CHECKC( _db.get(governance), gov_err::RECORD_NOT_FOUND, "governance not exist!" );
    CHECKC( (governance.last_updated_at + (governance.limit_update_hours * 3600)) <= current_time_point(), gov_err::NOT_MODIFY, "cannot be modified for now" );

    dao_info_t::idx_t info_tbl(MDAO_INFO, MDAO_INFO.value);
    const auto info = info_tbl.find(dao_code.value);
    CHECKC( (governance.proposal_model == propose_model_type::MIX && (has_auth(conf.managers[manager_type::PROPOSAL]) || has_auth(info->creator)))||
            (governance.proposal_model == propose_model_type::PROPOSAL && has_auth(conf.managers[manager_type::PROPOSAL]))
        , gov_err::NOT_MODIFY, "cannot be modified for now" );

    strategy_t::idx_t stg(MDAO_STG, MDAO_STG.value);
    auto vote_strategy = stg.find(vote_strategy_id);
    CHECKC( vote_strategy != stg.end(), gov_err::STRATEGY_NOT_FOUND, "strategy not found" );
    CHECKC( (vote_strategy->type != strategy_type::nftstake && vote_strategy->type != strategy_type::tokenstake) || 
            ((vote_strategy->type == strategy_type::nftstake || vote_strategy->type == strategy_type::tokenstake) && require_participation <= TEN_THOUSAND && require_pass <= TEN_THOUSAND), 
            gov_err::PARAM_ERROR, "param error");

    governance.last_updated_at                                             = current_time_point();
    governance.strategys[strategy_action_type::VOTE]                       = vote_strategy_id;
    governance.require_pass[strategy_action_type::VOTE.value]              = require_pass;
    governance.require_participation[strategy_action_type::VOTE.value]     = require_participation;

    _db.set(governance, _self);
}

ACTION mdaogov::setproposestg(const name& dao_code, const uint64_t& propose_strategy_id)
{
    auto conf = _conf();
    CHECKC( conf.status != conf_status::PENDING, gov_err::NOT_AVAILABLE, "under maintenance" );
    require_auth( conf.managers[manager_type::PROPOSAL] );

    governance_t governance(dao_code);
    CHECKC( _db.get(governance), gov_err::RECORD_NOT_FOUND, "governance not exist" );
    CHECKC( (governance.last_updated_at + (governance.limit_update_hours * 3600)) <= current_time_point(), gov_err::NOT_MODIFY, "cannot be modified for now" );

    strategy_t::idx_t stg(MDAO_STG, MDAO_STG.value);
    CHECKC(stg.find(propose_strategy_id) != stg.end(), gov_err::STRATEGY_NOT_FOUND, "strategy not found");

    governance.strategys[strategy_action_type::PROPOSAL]   = propose_strategy_id;
    governance.last_updated_at                             = current_time_point();//92142
    _db.set(governance, _self);
}

ACTION mdaogov::setlocktime(const name& dao_code, const uint16_t& limit_update_hours)
{
    auto conf = _conf();
    CHECKC( conf.status != conf_status::PENDING, gov_err::NOT_AVAILABLE, "under maintenance" );
    require_auth( conf.managers[manager_type::PROPOSAL] );

    governance_t governance(dao_code);
    CHECKC( _db.get(governance), gov_err::RECORD_NOT_FOUND, "governance not exist" );
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
    CHECKC( _db.get(governance), gov_err::RECORD_NOT_FOUND, "governance not exist" );
    CHECKC( voting_limit_hours <= governance.limit_update_hours , gov_err::TIME_LESS_THAN_ZERO, "lock time less than vote time" );
    CHECKC( (governance.last_updated_at + (governance.limit_update_hours * 3600)) <= current_time_point(), gov_err::NOT_MODIFY, "cannot be modified for now" );

    governance.voting_limit_hours = voting_limit_hours;
    governance.last_updated_at = current_time_point();
    _db.set(governance, _self);
}
 
ACTION mdaogov::startpropose(const name& creator, const name& dao_code, const string& title,
                                 const string& proposal_name, const string& desc, 
                                 const name& plan_type)
{
    require_auth( creator );
    auto conf = _conf();
    CHECKC( conf.status != conf_status::PENDING, gov_err::NOT_AVAILABLE, "under maintenance" );

    CHECKC( plan_type == plan_type::SINGLE || plan_type == plan_type::MULTIPLE, gov_err::TYPE_ERROR, "type error" );

    governance_t governance(dao_code);
    CHECKC( _db.get(governance) ,gov_err::RECORD_NOT_FOUND, "governance not exist" );
    uint64_t vote_strategy_id = governance.strategys.at(strategy_action_type::VOTE);
    uint64_t proposal_strategy_id = governance.strategys.at(strategy_action_type::PROPOSAL);

    strategy_t::idx_t stg(MDAO_STG, MDAO_STG.value);
    auto propose_strategy = stg.find(proposal_strategy_id);
    CHECKC( propose_strategy != stg.end(), gov_err::STRATEGY_NOT_FOUND, "strategy not found" );

    int64_t value = 0;
    _cal_votes(dao_code, *propose_strategy, creator, value);
    int32_t stg_weight = mdao::strategy::cal_weight(MDAO_STG, value, creator, proposal_strategy_id );
    CHECKC( stg_weight > 0, gov_err::INSUFFICIENT_WEIGHT, "insufficient strategy weight")

    VOTE_CREATE(MDAO_PROPOSAL, dao_code, creator, proposal_name, desc, title, vote_strategy_id, proposal_strategy_id, plan_type)
}

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
    CHECKC( (governance.last_updated_at + (governance.limit_update_hours * 3600)) <= current_time_point(), gov_err::NOT_MODIFY, "cannot be modified for now" );

    governance.proposal_model   = propose_model;
    governance.last_updated_at  = current_time_point();
    _db.set(governance, _self);
}

void mdaogov::_cal_votes(const name dao_code, const strategy_t& vote_strategy, const name voter, int64_t& value) {
    switch(vote_strategy.type.value){
        case strategy_type::tokenstake.value : {
            map<extended_symbol, int64_t> tokens = mdaostake::get_user_staked_tokens(MDAO_STAKE, voter, dao_code);
            value = tokens.at(extended_symbol{symbol(vote_strategy.ref_sym), vote_strategy.ref_contract});
            break;
        }
        case strategy_type::nftstake.value : {
            map<extended_symbol, int64_t> nfts = mdaostake::get_user_staked_tokens(MDAO_STAKE, voter, dao_code);
            value = nfts.at(extended_symbol{symbol(vote_strategy.ref_sym), vote_strategy.ref_contract});
            break;
        }
        case strategy_type::nparentstake.value:{
            set<extended_nsymbol> syms = amax::ntoken::get_syms_by_parent(vote_strategy.ref_contract, vote_strategy.ref_sym );
            map<extended_nsymbol, int64_t> nfts = mdaostake::get_user_staked_nfts(MDAO_STAKE, voter, dao_code);
            for (auto itr = syms.begin() ; itr != syms.end(); itr++) { 
               if(nfts.count(*itr)) value += nfts.at(*itr);
            }
            break;
         }
        default : {
            accounts accountstable(vote_strategy.ref_contract, voter.value);
            symbol sym = symbol(vote_strategy.ref_sym);
            const auto &ac = accountstable.get(sym.code().raw(), "account not found"); 
            value = ac.balance.amount;
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
