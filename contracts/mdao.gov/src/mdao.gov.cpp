#include <mdao.stg/mdao.stg.hpp>
#include <thirdparty/utils.hpp>
#include "mdao.gov/mdao.gov.hpp"
#include <mdao.info/mdao.info.db.hpp>
#include <mdao.stake/mdao.stake.db.hpp>
#include <set>

#define VOTE_CREATE(dao_code, creator, name, desc, title, vote_strategy_id, propose_strategy_id, type) \
{ action(permission_level{get_self(), "active"_n }, "mdao.propose"_n, "create"_n, std::make_tuple( dao_code, creator, name, desc, title, vote_strategy_id, propose_strategy_id, type)).send(); }

#define VOTE_EXCUTE(owner, proposeid) \
{ action(permission_level{get_self(), "active"_n }, "mdao.propose"_n, "create"_n, std::make_tuple( owner, proposeid)).send(); }

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

    // strategy_t::idx_t stg(MDAO_STG, MDAO_STG.value);
    // auto vote_strategy = stg.find(vote_strategy_id);
    // auto propose_strategy = stg.find(propose_strategy_id);
    // CHECKC( vote_strategy!= stg.end(), gov_err::STRATEGY_NOT_FOUND, "strategy not found");
    // CHECKC( propose_strategy != stg.end(), gov_err::STRATEGY_NOT_FOUND, "strategy not found");
    // CHECKC( (vote_strategy->type != strategy_type::nftstake && vote_strategy->type != strategy_type::tokenstake) || 
    //         ((vote_strategy->type == strategy_type::nftstake || vote_strategy->type == strategy_type::tokenstake) && require_participation <= TEN_THOUSAND && require_pass <= TEN_THOUSAND), 
    //         gov_err::STRATEGY_NOT_FOUND, "strategy not found");

    governance.dao_code                                                = dao_code;
    governance.strategys[strategy_action_type::PROPOSE]                = propose_strategy_id;
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
    CHECKC( !_db.get(governance), gov_err::CODE_REPEAT, "governance already existing!" );
    CHECKC( (governance.last_updated_at + (governance.limit_update_hours * 3600)) <= current_time_point(), gov_err::NOT_MODIFY, "cannot be modified for now" );

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

    governance.strategys[strategy_action_type::PROPOSE]    = propose_strategy_id;
    governance.last_updated_at                             = current_time_point();
    _db.set(governance, _self);
}



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
// amcli push action mdaogovtest1 startpropose '["ad", "amax.dao", 1, 1, 3, 2]' -p ad
ACTION mdaogov::startpropose(const name& creator, const name& dao_code, const string& title,
                                 const string& proposal_name, const string& desc, 
                                 const name& plan_type)
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
    uint64_t vote_strategy_id = governance.strategys.at(strategy_action_type::VOTE);
    uint64_t propose_strategy_id = governance.strategys.at(strategy_action_type::PROPOSE);

    strategy_t::idx_t stg(MDAO_STG, MDAO_STG.value);
    auto propose_strategy = stg.find(propose_strategy_id);
    CHECKC( propose_strategy != stg.end(), gov_err::STRATEGY_NOT_FOUND, "strategy not found" );

    int64_t value = 0;
    _cal_votes(dao_code, *propose_strategy, creator, value);
    int32_t stg_weight = mdao::strategy::cal_weight(MDAO_STG, value, creator, propose_strategy_id );
    CHECKC( stg_weight > 0, gov_err::INSUFFICIENT_WEIGHT, "insufficient strategy weight")

    VOTE_CREATE(dao_code, creator, proposal_name, desc, title, vote_strategy_id, propose_strategy_id, plan_type)

}

void mdaogov::_cal_votes(const name dao_code, const strategy_t& vote_strategy, const name voter, int64_t& value) {
    switch(vote_strategy.type.value){
        case strategy_type::tokenstake.value : {
            user_stake_t::idx_t user_token_stake(MDAO_STAKE, MDAO_STAKE.value);
            auto user_token_stake_index = user_token_stake.get_index<"unionid"_n>();
            auto user_token_stake_iter = user_token_stake_index.find(mdao::get_unionid(voter, dao_code));
            value = user_token_stake_iter->tokens_stake.at(extended_symbol{symbol(vote_strategy.ref_sym), vote_strategy.ref_contract});
            break;
        }
        case strategy_type::nftstake.value : {
            user_stake_t::idx_t user_nft_stake(MDAO_STAKE, MDAO_STAKE.value);
            auto user_nft_stake_index = user_nft_stake.get_index<"unionid"_n>();
            auto user_nft_stake_iter = user_nft_stake_index.find(mdao::get_unionid(voter, dao_code));
            value = user_nft_stake_iter->nfts_stake.at(extended_nsymbol{nsymbol(vote_strategy.ref_sym), vote_strategy.ref_contract});
            break;
        }
        default : {
            accounts accountstable(vote_strategy.ref_contract, voter.value);
            symbol sym = symbol(vote_strategy.ref_sym);
            const auto ac = accountstable.find(sym.code().raw()); 
            value = ac->balance.amount;
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
