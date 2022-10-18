#include <mdao.propose/mdao.propose.hpp>
#include <mdao.info/mdao.info.db.hpp>
#include <mdao.gov/mdao.gov.db.hpp>
#include <mdao.stg/mdao.stg.hpp>
#include <mdao.treasury/mdao.treasury.hpp>
#include <thirdparty/utils.hpp>
#include <set>

ACTION mdaoproposal::create(const name& dao_code, const name& creator, 
                            const string& proposal_name, const string& desc, 
                            const string& title, const uint64_t& vote_strategy_id, 
                            const name& type)
{
    auto conf = _conf();
    require_auth( conf.managers[manager_type::GOV] );

    proposal_t::idx_t proposal_tbl(_self, _self.value);
    auto id = proposal_tbl.available_primary_key();
    proposal_tbl.emplace( _self, [&]( auto& row ) {
        row.id                  =   id;
        row.dao_code            =   dao_code;
        row.vote_strategy_id    =   vote_strategy_id;
        row.creator             =   creator;
        row.status              =   proposal_status::CREATED;
        row.proposal_name	    =   proposal_name;
        row.desc	            =   desc;
        row.title	            =   title;
        row.type	            =   type;

   });

}

ACTION mdaoproposal::cancel(const name& owner, const uint64_t& proposal_id)
{
    require_auth( owner );

    auto conf = _conf();
    CHECKC( conf.status != conf_status::PENDING, proposal_err::NOT_AVAILABLE, "under maintenance" );

    proposal_t proposal(proposal_id);
    CHECKC( _db.get(proposal) ,proposal_err::RECORD_NOT_FOUND, "record not found" );
    CHECKC( owner == proposal.creator, proposal_err::PERMISSION_DENIED, "only the creator can operate" );
    CHECKC( proposal_status::CREATED == proposal.status || proposal_status::VOTING == proposal.status, proposal_err::STATUS_ERROR, "can only operate if the state is created and voting" );

    proposal.status  =  proposal_status::CANCELLED;
    _db.set(proposal, _self);
}

ACTION mdaoproposal::addplan( const name& owner, const uint64_t& proposal_id, 
                                const string& title, const string& desc )
{
    require_auth( owner );

    auto conf = _conf();
    CHECKC( conf.status != conf_status::PENDING, proposal_err::NOT_AVAILABLE, "under maintenance" );

    proposal_t proposal(proposal_id);
    CHECKC( _db.get(proposal) ,proposal_err::RECORD_NOT_FOUND, "record not found" );
    CHECKC( owner == proposal.creator, proposal_err::PERMISSION_DENIED, "only the creator can operate" );
    CHECKC( proposal_status::CREATED == proposal.status, proposal_err::STATUS_ERROR, "can only operate if the state is created" );
    
    if(proposal.type == plan_type::MULTIPLE){
        multiple_plan multi_plan = std::get<multiple_plan>(proposal.proposal_plan);
        auto prev_plan = multi_plan.plans.back();
        plan p;
        p.id           =   prev_plan.id++;
        p.title        =   title;
        p.desc         =   desc;
        multi_plan.plans.push_back(p);
    }else{
        single_plan s_plan;
        s_plan.title        =   title;
        s_plan.desc         =   desc;
        proposal.proposal_plan = s_plan;
    }

    _db.set(proposal, _self);
}

ACTION mdaoproposal::startvote(const name& owner, const uint64_t& proposal_id)
{
    require_auth( owner );

    auto conf = _conf();
    CHECKC( conf.status != conf_status::PENDING, proposal_err::NOT_AVAILABLE, "under maintenance" );

    proposal_t proposal(proposal_id);
    CHECKC( _db.get(proposal) ,proposal_err::RECORD_NOT_FOUND, "record not found" );
    CHECKC( proposal.status == proposal_status::CREATED, proposal_err::STATUS_ERROR, "proposal status must be created" );
    CHECKC( owner == proposal.creator, proposal_err::PERMISSION_DENIED, "only the creator can operate" );

    multiple_plan* multi_plan = std::get_if<multiple_plan>(&proposal.proposal_plan);
    single_plan* s_plan = std::get_if<single_plan>(&proposal.proposal_plan);
    CHECKC( (multi_plan != nullptr) || (s_plan != nullptr), proposal_err::PLANS_EMPTY, "please add plan" );

    proposal.status  =  proposal_status::VOTING;
    proposal.started_at = current_time_point();
    _db.set(proposal, _self);

}

ACTION mdaoproposal::execute(const name& owner, const uint64_t& proposal_id)
{
    auto conf = _conf();
    require_auth( owner );
    CHECKC( conf.status != conf_status::PENDING, proposal_err::NOT_AVAILABLE, "under maintenance" );

    proposal_t proposal(proposal_id);
    CHECKC( _db.get(proposal) ,proposal_err::RECORD_NOT_FOUND, "record not found" );
    CHECKC( owner == proposal.creator, proposal_err::PERMISSION_DENIED, "only the creator can operate" );
    CHECKC( proposal.status == proposal_status::VOTING, proposal_err::STATUS_ERROR, "proposal status must be running" );

    governance_t::idx_t governance_tbl(MDAO_GOV, MDAO_GOV.value);
    const auto governance = governance_tbl.find(proposal.dao_code.value);
    CHECKC( (proposal.started_at + (governance->voting_limit_hours * 3600)) <= current_time_point(), proposal_err::ALREADY_EXPIRED, "proposal is already expired" );
    // CHECKC( (proposal.recv_votes - proposal.reject_votes) >= governance->require_pass, proposal_err::VOTES_NOT_ENOUGH, "votes must meet the minimum number of votes" );

    strategy_t::idx_t stg(MDAO_STG, MDAO_STG.value);
    auto vote_strategy = stg.find(proposal.vote_strategy_id);
    if((vote_strategy->type != strategy_type::nftstaking && vote_strategy->type != strategy_type::tokenstaking)){
        CHECKC( proposal.recv_votes >= governance->require_pass.at(proposal.vote_strategy_id), proposal_err::VOTES_NOT_ENOUGH, "votes must meet the minimum number of votes" );
        CHECKC( proposal.recv_votes >= governance->require_participation.at(proposal.vote_strategy_id), proposal_err::VOTES_NOT_ENOUGH, "votes must meet the minimum number of votes" );
    }
    // else{

    // }

    if(proposal.type == plan_type::SINGLE){
        single_plan s_plan = std::get<single_plan>(proposal.proposal_plan);
        s_plan.execute_action.send();
    }

    proposal.status  =  proposal_status::EXECUTED;
    proposal.executed_at = current_time_point();
    _db.set(proposal, _self);
}

ACTION mdaoproposal::votefor(const name& voter, const uint64_t& proposal_id, 
                                const uint32_t plan_id, const bool direction)
{
    require_auth( voter );

    auto conf = _conf();
    CHECKC( conf.status != conf_status::PENDING, proposal_err::NOT_AVAILABLE, "under maintenance" );

    proposal_t proposal(proposal_id);
    CHECKC( _db.get(proposal) ,proposal_err::RECORD_NOT_FOUND, "proposal not found" );
    CHECKC( proposal.status == proposal_status::VOTING, proposal_err::STATUS_ERROR, "proposal status must be running" );

    governance_t::idx_t governance_tbl(MDAO_GOV, MDAO_GOV.value);
    const auto governance = governance_tbl.find(proposal.dao_code.value);
    CHECKC( (proposal.started_at + (governance->voting_limit_hours * 3600)) <= current_time_point(), proposal_err::ALREADY_EXPIRED, "proposal is already expired" );

    votelist_t::idx_t vote_tbl(_self, _self.value);
    auto vote_index = vote_tbl.get_index<"unionid"_n>();
    uint128_t union_id = get_union_id(voter,proposal_id);
    CHECKC( vote_index.find(union_id) == vote_index.end() ,proposal_err::VOTED, "account have voted" );

    strategy_t::idx_t stg(MDAO_STG, MDAO_STG.value);
    auto vote_strategy = stg.find(proposal.vote_strategy_id);
    int64_t value;
    if((vote_strategy->type != strategy_type::nftstaking && vote_strategy->type != strategy_type::tokenstaking)){
        accounts accountstable(vote_strategy->ref_contract, voter.value);
        const auto ac = accountstable.find(vote_strategy->require_symbol_code.raw()); 
        value = ac->balance.amount;
    }
    // else{

    // }

    int stg_weight = mdao::strategy::cal_weight(MDAO_STG, value, voter, proposal.vote_strategy_id);
    CHECKC( stg_weight > 0, proposal_err::INSUFFICIENT_VOTES, "insufficient votes" );

    auto id = vote_tbl.available_primary_key();
    vote_tbl.emplace( _self, [&]( auto& row ) {
        row.id          =   id;
        row.account     =   voter;
        row.proposal_id =   proposal_id;
        row.direction   =   direction ? vote_direction::AGREE : vote_direction::REJECT;
        row.vote_weight   =   stg_weight;
        row.voted_at      =   current_time_point();
    });

    if(proposal.type == plan_type::SINGLE){
        single_plan s_plan = std::get<single_plan>(proposal.proposal_plan);
        s_plan.recv_votes = direction ? s_plan.recv_votes + stg_weight : s_plan.recv_votes;
    }else{
        multiple_plan s_plan = std::get<multiple_plan>(proposal.proposal_plan);
        for( vector<plan>::iterator plan_iter = s_plan.plans.begin(); plan_iter != s_plan.plans.end(); plan_iter++ ){
            if( plan_id == plan_iter->id ){

                plan_iter->recv_votes = direction ? plan_iter->recv_votes + stg_weight : plan_iter->recv_votes;
                break;
            }
        }
    }
    if(!direction) {
        proposal.reject_votes += stg_weight;
        proposal.reject_users_count++;
    }
    proposal.recv_votes += stg_weight;
    proposal.users_count++;
    _db.set(proposal, _self);
}

ACTION mdaoproposal::setaction(const name& owner, const uint64_t& proposal_id,
                                const uint32_t& plan_id,  const name& action_name,
                                const name& action_account, const std::vector<char>& packed_action_data)
{
    require_auth(owner);

    auto conf = _conf();
    CHECKC( conf.status != conf_status::PENDING, proposal_err::NOT_AVAILABLE, "under maintenance" );

    proposal_t proposal(proposal_id);
    CHECKC( _db.get(proposal) ,proposal_err::RECORD_NOT_FOUND, "record not found" );
    CHECKC( owner == proposal.creator, proposal_err::PERMISSION_DENIED, "only the creator can operate" );
    CHECKC( plan_type::SINGLE == proposal.type, proposal_err::PERMISSION_DENIED, "only type SINGLE can be used" );

    governance_t::idx_t governance_tbl(MDAO_GOV, MDAO_GOV.value);
    const auto governance = governance_tbl.find(proposal.dao_code.value);
    CHECKC( (proposal.started_at + (governance->voting_limit_hours * 3600)) <= current_time_point(), proposal_err::ALREADY_EXPIRED, "proposal is already expired" );

    single_plan* s_plan = std::get_if<single_plan>(&proposal.proposal_plan);
    CHECKC( s_plan != nullptr, proposal_err::PLANS_EMPTY, "please add plan" );

    permission_level pem({_self, "active"_n});

    single_plan plan = std::get<single_plan>(proposal.proposal_plan);

    switch (action_name.value)
    {
        case proposal_action_type::updatedao.value: {
            updatedao_data action_data = unpack<updatedao_data>(packed_action_data);
            action_data_variant data_var = action_data;
            _check_proposal_params(data_var, action_name, action_account, conf);
            plan.execute_action = action(pem, action_account, action_name, action_data);
            break;
        }
        case proposal_action_type::bindtoken.value: {
            bindtoken_data action_data = unpack<bindtoken_data>(packed_action_data);
            action_data_variant data_var = action_data;
            _check_proposal_params(data_var, action_name, action_account, conf);
            plan.execute_action = action(pem, action_account, action_name, action_data);
            break;
        }
        case proposal_action_type::binddapp.value: {
            binddapp_data action_data = unpack<binddapp_data>(packed_action_data);
            action_data_variant data_var = action_data;
            _check_proposal_params(data_var, action_name, action_account, conf);
            plan.execute_action = action(pem, action_account, action_name, action_data);
            break;
        }
        case proposal_action_type::createtoken.value: {
            createtoken_data action_data = unpack<createtoken_data>(packed_action_data);
            action_data_variant data_var = action_data;
            _check_proposal_params(data_var, action_name, action_account, conf);
            plan.execute_action = action(pem, action_account, action_name, action_data);
            break;
        }
        case proposal_action_type::issuetoken.value: {
            issuetoken_data action_data = unpack<issuetoken_data>(packed_action_data);
            action_data_variant data_var = action_data;
            _check_proposal_params(data_var, action_name, action_account, conf);
            plan.execute_action = action(pem, action_account, action_name, action_data);
            break;
        }
        case proposal_action_type::setvotestg.value: {
            setvotestg_data action_data = unpack<setvotestg_data>(packed_action_data);
            action_data_variant data_var = action_data;
            _check_proposal_params(data_var, action_name, action_account, conf);
            plan.execute_action = action(pem, action_account, action_name, action_data);
            break;
        }
        case proposal_action_type::setproposestg.value: {
            setproposestg_data action_data = unpack<setproposestg_data>(packed_action_data);
            action_data_variant data_var = action_data;
            _check_proposal_params(data_var, action_name, action_account, conf);
            plan.execute_action = action(pem, action_account, action_name, action_data);
            break;
        }
        case proposal_action_type::setlocktime.value: {
            setlocktime_data action_data = unpack<setlocktime_data>(packed_action_data);
            action_data_variant data_var = action_data;
            _check_proposal_params(data_var, action_name, action_account, conf);
            plan.execute_action = action(pem, action_account, action_name, action_data);

            break;
        }
        case proposal_action_type::tokentranout.value: {
            tokentranout_data action_data = unpack<tokentranout_data>(packed_action_data);
            action_data_variant data_var = action_data;
            _check_proposal_params(data_var, action_name, action_account, conf);
            plan.execute_action = action(pem, action_account, action_name, action_data);

            break;
        }
        default: {
            CHECKC( false, err::PARAM_ERROR, "Unsupport proposal type")
            break;
        }
    }


    _db.set(proposal, _self);
}

void mdaoproposal::_check_proposal_params(const action_data_variant& data_var,  const name& action_name, const name& action_account, const conf_t& conf)
{

    switch (action_name.value){
        case proposal_action_type::updatedao.value:{
            updatedao_data data = std::get<updatedao_data>(data_var);

            dao_info_t::idx_t info_tbl(MDAO_INFO, MDAO_INFO.value);
            const auto info = info_tbl.find(data.code.value);
            CHECKC(info != info_tbl.end(), proposal_err::RECORD_NOT_FOUND, "record not found");

            CHECKC(info->creator == data.owner, proposal_err::PERMISSION_DENIED, "only the creator can operate");
            CHECKC(!data.groupid.empty(), proposal_err::PARAM_ERROR, "groupid can not be empty");
            CHECKC(!(data.symcode.empty() ^ data.symcontract.empty()), proposal_err::PARAM_ERROR, "symcode and symcontract must be null or not null");

            if( !data.symcode.empty() ){
                accounts accountstable(name(data.symcontract), data.owner.value);
                const auto ac = accountstable.find(symbol_code(data.symcode).raw());
                CHECKC(ac != accountstable.end(),  proposal_err::SYMBOL_ERROR, "symcode or symcontract not found");
            }

            break;
        }
        case proposal_action_type::bindtoken.value: {
            bindtoken_data data = std::get<bindtoken_data>(data_var);

            dao_info_t::idx_t info_tbl(MDAO_INFO, MDAO_INFO.value);
            const auto info = info_tbl.find(data.code.value);
            CHECKC(info != info_tbl.end(), proposal_err::RECORD_NOT_FOUND, "record not found");
            CHECKC(info->creator == data.owner, proposal_err::PERMISSION_DENIED, "only the creator can operate");

            break;
        }
        case proposal_action_type::binddapp.value: {
            binddapp_data data = std::get<binddapp_data>(data_var);

            dao_info_t::idx_t info_tbl(MDAO_INFO, MDAO_INFO.value);
            const auto info = info_tbl.find(data.code.value);
            CHECKC(info != info_tbl.end(), proposal_err::RECORD_NOT_FOUND, "record not found");
            CHECKC(info->creator == data.owner, proposal_err::PERMISSION_DENIED, "only the creator can operate");
            CHECKC( data.dapps.size() != 0 ,proposal_err::CANNOT_ZERO, "dapp size cannot be zero" );

            break;
        }
        case proposal_action_type::createtoken.value: {
            createtoken_data data = std::get<createtoken_data>(data_var);

            CHECKC( data.fullname.size() <= 20, proposal_err::SIZE_TOO_MUCH, "fullname has more than 20 bytes")
            CHECKC( data.maximum_supply.amount > 0, proposal_err::NOT_POSITIVE, "not positive quantity:" + data.maximum_supply.to_string() )
            symbol_code supply_code = data.maximum_supply.symbol.code();
            CHECKC( supply_code.length() > 3, proposal_err::NO_AUTH, "cannot create limited token" )
            CHECKC( !conf.black_symbols.count(supply_code) ,proposal_err::NOT_ALLOW, "token not allowed to create" );

            stats statstable( MDAO_TOKEN, supply_code.raw() );
            CHECKC( statstable.find(supply_code.raw()) == statstable.end(), proposal_err::CODE_REPEAT, "token already exist")

            dao_info_t::idx_t info_tbl(MDAO_INFO, MDAO_INFO.value);
            const auto info = info_tbl.find(data.code.value);
            CHECKC(info != info_tbl.end(), proposal_err::RECORD_NOT_FOUND, "record not found");
            CHECKC(info->creator == data.owner, proposal_err::PERMISSION_DENIED, "only the creator can operate");

            break;
        }
        case proposal_action_type::issuetoken.value: {
            issuetoken_data data = std::get<issuetoken_data>(data_var);

            symbol_code supply_code = data.quantity.symbol.code();
            stats statstable( MDAO_TOKEN, supply_code.raw() );
            CHECKC( statstable.find(supply_code.raw()) != statstable.end(), proposal_err::TOKEN_NOT_EXIST, "token not exist")

            dao_info_t::idx_t info_tbl(MDAO_INFO, MDAO_INFO.value);
            const auto info = info_tbl.find(data.code.value);
            CHECKC(info != info_tbl.end(), proposal_err::RECORD_NOT_FOUND, "record not found");

            break;
        }
        case proposal_action_type::setvotestg.value: {
            setvotestg_data data = std::get<setvotestg_data>(data_var);
            
            governance_t governance(data.dao_code);
            CHECKC( !_db.get(governance), proposal_err::CODE_REPEAT, "governance already existing!" );
            CHECKC( (governance.last_updated_at + (governance.limit_update_hours * 3600)) <= current_time_point(), proposal_err::NOT_MODIFY, "cannot be modified for now" );

            strategy_t::idx_t stg(MDAO_STG, MDAO_STG.value);
            auto vote_strategy = stg.find(data.vote_strategy_id);
            CHECKC( vote_strategy != stg.end(), proposal_err::STRATEGY_NOT_FOUND, "strategy not found" );

            CHECKC( (vote_strategy->type != strategy_type::nftstaking && vote_strategy->type != strategy_type::tokenstaking) || 
                    ((vote_strategy->type == strategy_type::nftstaking || vote_strategy->type == strategy_type::tokenstaking) && data.require_participation <= 100000 && data.require_pass <= 100000), 
            proposal_err::PARAM_ERROR, "param error");

            break;
        }
        case proposal_action_type::setproposestg.value:{
            setproposestg_data data = std::get<setproposestg_data>(data_var);

            strategy_t::idx_t stg(MDAO_STG, MDAO_STG.value);
            CHECKC(stg.find(data.propose_strategy_id) != stg.end(), proposal_err::STRATEGY_NOT_FOUND, "strategy not found:"+to_string(data.propose_strategy_id) );

            break;
        }
        case proposal_action_type::setlocktime.value: {
            setlocktime_data data = std::get<setlocktime_data>(data_var);

            governance_t governance(data.dao_code);
            CHECKC( !_db.get(governance), proposal_err::CODE_REPEAT, "governance already existing!" );
            CHECKC( data.limit_update_hours >= governance.voting_limit_hours , proposal_err::TIME_LESS_THAN_ZERO, "lock time less than vote time" );
            CHECKC( (governance.last_updated_at + (governance.limit_update_hours * 3600)) <= current_time_point(), proposal_err::NOT_MODIFY, "cannot be modified for now" );
            
            break;
        }
        case proposal_action_type::tokentranout.value: {
            tokentranout_data data = std::get<tokentranout_data>(data_var);
            CHECKC( data.quantity.quantity.amount > 0, proposal_err::NOT_POSITIVE, "quantity must be positive" )

            treasury_balance_t treasury_balance(data.dao_code);
            CHECKC( _db.get(treasury_balance), proposal_err::RECORD_NOT_FOUND, "dao not found" )

            uint64_t amount = treasury_balance.stake_assets[data.quantity.get_extended_symbol()];
            CHECKC( amount > data.quantity.quantity.amount, proposal_err::INSUFFICIENT_BALANCE, "not sufficient funds " )

            break;
        }
        default:{
            CHECKC(false, err::PARAM_ERROR, "Unsupport proposal type")
            break;
        }
    }
}

void mdaoproposal::recycledb(uint32_t max_rows) {
    require_auth( _self );
    proposal_t::idx_t proposal_tbl(_self, _self.value);
    auto proposal_itr = proposal_tbl.begin();
    for (size_t count = 0; count < max_rows && proposal_itr != proposal_tbl.end(); count++) {
        proposal_itr = proposal_tbl.erase(proposal_itr);
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

