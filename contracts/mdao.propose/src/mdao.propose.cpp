#include <mdao.propose/mdao.propose.hpp>
#include <mdao.info/mdao.info.db.hpp>
#include <mdao.gov/mdao.gov.hpp>
#include <mdao.treasury/mdao.treasury.hpp>
#include <mdao.stake/mdao.stake.hpp>
#include <thirdparty/utils.hpp>
#include <set>


ACTION mdaoproposal::init(const uint64_t& last_propose_id, const uint64_t& last_vote_id)
{
    require_auth( _self );
    _gstate.last_propose_id = last_propose_id;
    _gstate.last_vote_id    = last_vote_id;
    _global.set( _gstate, get_self() );
}

ACTION mdaoproposal::removeglobal( )
{
    require_auth( _self );
    _global.remove();
}

ACTION mdaoproposal::create(const name& creator, const name& dao_code, const string& title, const string& desc, map<string, string> options)
{
    require_auth( creator );
    auto conf = _conf();
    CHECKC( conf.status != conf_status::PENDING, gov_err::NOT_AVAILABLE, "under maintenance" );
    CHECKC( title.size() > 0 && title.size() <= 32, proposal_err::INVALID_FORMAT, "title length is more than 32 bytes and less than 0 bytes");
    CHECKC( desc.size() <= 224, proposal_err::INVALID_FORMAT, "desc length is more than 224 bytes");
    proposal_t proposal(_gstate.last_propose_id);
    
    for (auto option : options) {
        CHECKC( option.second.size() > 0 && option.second.size() <= 32, proposal_err::INVALID_FORMAT, "option title length is more than 32 bytes and less than 0 bytes");
        CHECKC( option.first.size() > 0, proposal_err::INVALID_FORMAT, "option key length is less than 0 bytes");
        proposal.options[option.first] = {option.second, 0};
    }
      
    governance_t::idx_t governance(MDAO_GOV, MDAO_GOV.value);
    auto gov = governance.find(dao_code.value);
    CHECKC( gov != governance.end(), proposal_err::RECORD_NOT_FOUND, "governance not found" );
    uint64_t vote_strategy_id = gov->strategies.at(strategy_action_type::VOTE);
    uint64_t proposal_strategy_id = gov->strategies.at(strategy_action_type::PROPOSAL);

    strategy_t::idx_t stg(MDAO_STG, MDAO_STG.value);
    auto propose_strategy = stg.find(proposal_strategy_id);
    CHECKC( propose_strategy != stg.end(), proposal_err::RECORD_NOT_FOUND, "strategy not found" );   

    weight_struct weight_str;
    _cal_votes(dao_code, *propose_strategy, creator, weight_str, 0);
    CHECKC( weight_str.weight > 0, proposal_err::INSUFFICIENT_BALANCE, "insufficient strategy weight")

    proposal.dao_code            =   dao_code;
    proposal.vote_strategy_id    =   vote_strategy_id;
    proposal.proposal_strategy_id=   proposal_strategy_id;
    proposal.creator             =   creator;
    proposal.status              =   proposal_status::CREATED;
    proposal.desc	               =   desc;
    proposal.title	             =   title;
    proposal.ended_at	           =   time_point_sec(current_time_point()) + (gov->voting_period * second_per_day);
    _db.set(proposal, creator);
    
    _gstate.last_propose_id++;
    _global.set( _gstate, get_self() );
}

ACTION mdaoproposal::cancel(const name& owner, const uint64_t& proposal_id)
{
    require_auth( owner );

    auto conf = _conf();
    CHECKC( conf.status != conf_status::PENDING, proposal_err::NOT_AVAILABLE, "under maintenance" );

    proposal_t proposal(proposal_id);
    CHECKC( _db.get(proposal) ,proposal_err::RECORD_NOT_FOUND, "record not found" );
    CHECKC( owner == proposal.creator, proposal_err::PERMISSION_DENIED, "only the creator can operate" );
    CHECKC( proposal_status::CREATED == proposal.status, proposal_err::STATUS_ERROR, "can only operate if the state is created and voting" );
    
    // governance_t::idx_t governance_tbl(MDAO_GOV, MDAO_GOV.value);
    // const auto governance = governance_tbl.find(proposal.dao_code.value);
    // CHECKC( ((proposal.created_at + (governance->voting_period * second_per_day)) >= current_time_point()), 
    //             proposal_err::ALREADY_EXPIRED, "the voting cycle is over. it can't be canceled" );

    _db.del(proposal);
}


ACTION mdaoproposal::votefor(const name& voter, const uint64_t& proposal_id, 
                                const string& option_key)
{
    require_auth( voter );

    auto conf = _conf();
    CHECKC( conf.status != conf_status::PENDING, proposal_err::NOT_AVAILABLE, "under maintenance" );

    proposal_t proposal(proposal_id);
    CHECKC( _db.get(proposal) ,proposal_err::RECORD_NOT_FOUND, "proposal not found" );
    CHECKC( proposal.status == proposal_status::VOTING || proposal.status == proposal_status::CREATED, proposal_err::STATUS_ERROR, "proposal status must be running" );
    CHECKC( proposal.options.count(option_key), proposal_err::PARAM_ERROR, "param error" );

    bool is_not_expired = proposal.ended_at >= current_time_point();
    if ( is_not_expired ) {
        vote_t::idx_t vote_tbl(_self, _self.value);
        auto vote_index = vote_tbl.get_index<"unionid"_n>();
        uint128_t union_id = get_union_id(voter, proposal_id);
        CHECKC( vote_index.find(union_id) == vote_index.end() ,proposal_err::VOTED, "account have voted" );

        strategy_t::idx_t stg(MDAO_STG, MDAO_STG.value);
        auto vote_strategy = stg.find(proposal.vote_strategy_id);

        weight_struct weight_str;
        _cal_votes(proposal.dao_code, *vote_strategy, voter, weight_str, conf.stake_period_days * second_per_day);
        CHECKC( weight_str.weight > 0, proposal_err::INSUFFICIENT_VOTES, "insufficient votes" );

        vote_tbl.emplace( voter, [&]( auto& row ) {
            row.id            =   _gstate.last_vote_id++;
            row.account       =   voter;
            row.proposal_id   =   proposal_id;
            row.vote_weight   =   weight_str.weight;
            row.quantity      =   weight_str.quantity;
            row.stg_type      =   vote_strategy->type;
            row.voted_at      =   current_time_point();
            row.option_key    =   option_key;

        });

        proposal.options[option_key].recv_votes = proposal.options[option_key].recv_votes + weight_str.weight;
        proposal.status = proposal.status != proposal_status::VOTING ? proposal_status::VOTING :  proposal.status;

        _db.set(proposal, _self);
        _global.set( _gstate, get_self() ); 
    } else {
        proposal.status = proposal_status::EXPIRED;
        _db.set(proposal, _self);
    }
    
}

// void mdaoproposal::deletepropose(uint64_t id) {
//     proposal_t proposal(id);
//     _db.del(proposal);
// }
// void mdaoproposal::deletevote(uint32_t id) {
//     vote_t vote(id);
//     vote.id = id;
//     _db.del(vote);
// }

void mdaoproposal::withdraw(const vector<withdraw_str>& withdraws) {

    auto conf = _conf();
    CHECKC( conf.status != conf_status::PENDING, proposal_err::NOT_AVAILABLE, "under maintenance" );
    require_auth(conf.admin);

    CHECKC( withdraws.size() > 0, proposal_err::PARAM_ERROR, "withdraws size must be more than 0" );
    
    for (auto& w : withdraws) {
        vote_t vote;
        vote.id = w.vote_id;
        CHECKC( _db.get(vote) ,proposal_err::NOT_VOTED, "account not voted" );
        CHECKC( vote.stg_type == strategy_type::TOKEN_BALANCE || vote.stg_type == strategy_type::NFT_BALANCE || vote.stg_type == strategy_type::TOKEN_STAKE, proposal_err::NO_SUPPORT, "no support withdraw" );
        
        proposal_t proposal(vote.proposal_id);
        CHECKC( _db.get(proposal), proposal_err::RECORD_NOT_FOUND, "proposal not found" );
        CHECKC( proposal.status == proposal_status::VOTING, proposal_err::STATUS_ERROR, "proposal status must be running" );
        
        assert( proposal.options[w.option_key].recv_votes >= vote.vote_weight );
        proposal.options[w.option_key].recv_votes -= vote.vote_weight;
    
        _db.set(proposal, _self);
        _db.del(vote);
    }
     
}

void mdaoproposal::_cal_votes(const name dao_code, const strategy_t& vote_strategy, const name voter, weight_struct& weight_str, const uint32_t& lock_time) {
    switch(vote_strategy.type.value){
        case strategy_type::TOKEN_STAKE.value :{
            weight_str = mdao::strategy::cal_stake_weight(MDAO_STG, vote_strategy.id, dao_code, MDAO_STAKE, voter);
            
            asset quantity = std::get<asset>(weight_str.quantity);
            if(quantity.symbol != symbol("AMAX",8) && lock_time > 0 && weight_str.weight > 0){
                user_stake_t::idx_t user_stake(MDAO_STAKE, MDAO_STAKE.value); 
                auto user_stake_index = user_stake.get_index<"unionid"_n>(); 
                auto user_stake_iter = user_stake_index.find(mdao::get_unionid(voter, dao_code)); 
                CHECKC( user_stake_iter != user_stake_index.end(), proposal_err::RECORD_NOT_FOUND, "stake record not exist" );

                EXTEND_LOCK(MDAO_STAKE, MDAO_GOV, user_stake_iter->id, lock_time);
            }

            break;
        } 
        case strategy_type::NFT_STAKE.value : 
        case strategy_type::NFT_PARENT_STAKE.value:{
            weight_str = mdao::strategy::cal_stake_weight(MDAO_STG, vote_strategy.id, dao_code, MDAO_STAKE, voter);
            
            if(lock_time > 0 && weight_str.weight > 0){
                user_stake_t::idx_t user_stake(MDAO_STAKE, MDAO_STAKE.value); 
                auto user_stake_index = user_stake.get_index<"unionid"_n>(); 
                auto user_stake_iter = user_stake_index.find(mdao::get_unionid(voter, dao_code)); 
                CHECKC( user_stake_iter != user_stake_index.end(), proposal_err::RECORD_NOT_FOUND, "stake record not exist" );

                EXTEND_LOCK(MDAO_STAKE, MDAO_GOV, user_stake_iter->id, lock_time);
            }

            break;
        }
        case strategy_type::TOKEN_BALANCE.value:{
            weight_str = mdao::strategy::cal_balance_weight(MDAO_STG, vote_strategy.id, voter);
            break;
        }
        case strategy_type::NFT_BALANCE.value:
        case strategy_type::NFT_PARENT_BALANCE.value:
        case strategy_type::TOKEN_SUM.value: {
            weight_str = mdao::strategy::cal_balance_weight(MDAO_STG, vote_strategy.id, voter);
            break;
        }
        default : {
           CHECKC( false, gov_err::TYPE_ERROR, "type error" );
        }
    }
}

const mdaoproposal::conf_t& mdaoproposal::_conf() {
    if (!_conf_ptr) {
        _conf_tbl_ptr = make_unique<conf_table_t>(MDAO_CONF, MDAO_CONF.value);
        CHECKC(_conf_tbl_ptr->exists(), proposal_err::SYSTEM_ERROR, "conf table not existed in contract" );
        _conf_ptr = make_unique<conf_t>(_conf_tbl_ptr->get());
    }
    return *_conf_ptr;
}

