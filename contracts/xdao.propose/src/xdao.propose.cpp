#include <xdao.propose/xdao.propose.hpp>
#include <xdao.info/xdao.info.db.hpp>
#include <xdao.gov/xdao.gov.db.hpp>
#include <xdao.stg/xdao.stg.hpp>
#include <thirdparty/utils.hpp>
#include <set>

ACTION xdaopropose::create(const name& creator,const string& name, const string& desc,
                            const uint64_t& stgid, const uint32_t& votes)
{
    auto conf = _conf();
    require_auth( conf.managers[manager::GOV] );
    
    propose_t::idx_t proposes(_self, _self.value);
    auto id = proposes.available_primary_key();
    proposes.emplace( _self, [&]( auto& row ) {
        row.id          =   id;
        row.creator     =   creator;
        row.name        =   name;
        row.status      =   propose_status::CREATED;
        row.desc	    =   desc;
        row.vote_stgid	=   stgid;
        row.req_votes	=   votes;
        row.vote_type	=   vote_type::PLEDGE;

   });

}

ACTION xdaopropose::cancel(const name& owner, const uint64_t& proposeid)
{
    require_auth( owner );

    auto conf = _conf();
    CHECKC( conf.status != conf_status::MAINTAIN, propose_err::NOT_AVAILABLE, "under maintenance" );

    propose_t propose(proposeid);
    CHECKC( _db.get(propose) ,propose_err::RECORD_NOT_FOUND, "record not found" );
    CHECKC( owner == propose.creator, propose_err::PERMISSION_DENIED, "only the creator can operate" );

    propose.status  =  propose_status::CANCELLED;
    _db.set(propose, _self);
}

ACTION xdaopropose::addoption( const name& owner, const uint64_t& proposeid, const string& title )
{
    require_auth( owner );

    auto conf = _conf();
    CHECKC( conf.status != conf_status::MAINTAIN, propose_err::NOT_AVAILABLE, "under maintenance" );

    propose_t propose(proposeid);
    CHECKC( _db.get(propose) ,propose_err::RECORD_NOT_FOUND, "record not found" );
    CHECKC( owner == propose.creator, propose_err::PERMISSION_DENIED, "only the creator can operate" );

    auto prev_opt = propose.opts.back();
    option opt;
    opt.id           =   prev_opt.id++;
    opt.title        =   title;
    propose.opts.push_back(opt);
    _db.set(propose, _self);

}

ACTION xdaopropose::start(const name& owner, const uint64_t& proposeid)
{
    require_auth( owner );

    auto conf = _conf();
    CHECKC( conf.status != conf_status::MAINTAIN, propose_err::NOT_AVAILABLE, "under maintenance" );

    propose_t propose(proposeid);
    CHECKC( _db.get(propose) ,propose_err::RECORD_NOT_FOUND, "record not found" );
    CHECKC( propose.status == propose_status::CREATED, propose_err::STATUS_ERROR, "propose status must be created" );
    CHECKC( owner == propose.creator, propose_err::PERMISSION_DENIED, "only the creator can operate" );
    CHECKC( !propose.opts.empty(), propose_err::OPTS_EMPTY, "please add option" );

    propose.status  =  propose_status::RUNNING;
    _db.set(propose, _self);

}

ACTION xdaopropose::excute(const name& owner, const uint64_t& proposeid)
{
    auto conf = _conf();
    require_auth( conf.managers[manager::GOV] );

    CHECKC( conf.status != conf_status::MAINTAIN, propose_err::NOT_AVAILABLE, "under maintenance" );

    propose_t propose(proposeid);
    CHECKC( _db.get(propose) ,propose_err::RECORD_NOT_FOUND, "record not found" );
    CHECKC( owner == propose.creator, propose_err::PERMISSION_DENIED, "only the creator can operate" );
    CHECKC( propose.status == propose_status::RUNNING, propose_err::STATUS_ERROR, "propose status must be running" );
    for( vector<option>::iterator opt_iter = propose.opts.begin(); opt_iter != propose.opts.end(); opt_iter++ ){
        CHECKC( (*opt_iter).recv_votes >= propose.req_votes, propose_err::INSUFFICIENT_VOTES, "insufficient votes" );
    }
    propose.status  =  propose_status::EXCUTING;
    _db.set(propose, _self);
}

ACTION xdaopropose::votefor(const name& voter, const uint64_t& proposeid, const uint32_t optid)
{
    require_auth( voter );

    auto conf = _conf();
    CHECKC( conf.status != conf_status::MAINTAIN, propose_err::NOT_AVAILABLE, "under maintenance" );

    propose_t propose(proposeid);
    CHECKC( _db.get(propose) ,propose_err::RECORD_NOT_FOUND, "propose not found" );

    vote_t::idx_t votes(_self, _self.value);
    auto vote_index = votes.get_index<"unionid"_n>();
    uint128_t union_id = get_union_id(voter,proposeid);
    CHECKC( vote_index.find(union_id) == vote_index.end() ,propose_err::VOTED, "account have voted" );

    int stg_weight = xdao::strategy::cal_stg_weight(XDAO_STG, voter, propose.vote_stgid);
    CHECKC( stg_weight > 0, propose_err::INSUFFICIENT_VOTES, "insufficient votes" );

    auto id = votes.available_primary_key();
    votes.emplace( _self, [&]( auto& row ) {
        row.id          =   id;
        row.account     =   voter;
        row.proposal_id =   proposeid;
        row.vote_time   =   stg_weight;
    });

    for( vector<option>::iterator opt_iter = propose.opts.begin(); opt_iter != propose.opts.end(); opt_iter++ ){
        if( optid == (*opt_iter).id ){
            (*opt_iter).recv_votes  +=  stg_weight;
            break;
        }
    }

    _db.set(propose, _self);
}

ACTION xdaopropose::setaction(const name& owner, const uint64_t& proposeid,
                                const uint32_t& optid,  const name& action_name,
                                const name& action_account, const std::vector<char>& packed_action_data)
{
    require_auth(owner);

    auto conf = _conf();
    CHECKC( conf.status != conf_status::MAINTAIN, propose_err::NOT_AVAILABLE, "under maintenance" );

    propose_t propose(proposeid);
    CHECKC( _db.get(propose) ,propose_err::RECORD_NOT_FOUND, "record not found" );
    CHECKC( owner == propose.creator, propose_err::PERMISSION_DENIED, "only the creator can operate" );

    permission_level pem({_self, "active"_n});

    option opt;
    for (vector<option>::iterator opt_iter = propose.opts.begin(); opt_iter != propose.opts.end(); opt_iter++){
        if (optid == (*opt_iter).id){
            opt = *opt_iter;
            break;
        }
    }

    switch (action_name.value)
    {
        case proposal_action_type::updatedao.value: {
            updatedao_data action_data = unpack<updatedao_data>(packed_action_data);
            action_data_variant data_var = action_data;
            _check_proposal_params(data_var, action_name, action_account);
            opt.excute_action = action(pem, action_account, action_name, action_data);
            break;
        }
        case proposal_action_type::setpropstg.value: {
            setpropstg_data action_data = unpack<setpropstg_data>(packed_action_data);
            action_data_variant data_var = action_data;
            _check_proposal_params(data_var, action_name, action_account);
            opt.excute_action = action(pem, action_account, action_name, action_data);
            break;
        }
        default: {
            CHECKC( false, err::PARAM_ERROR, "Unsupport proposal type")
            break;
        }
    }

    _db.set(propose, _self);
}

void xdaopropose::_check_proposal_params(const action_data_variant& data_var,  const name& action_name, const name& action_account)
{

    switch (action_name.value){
        case proposal_action_type::updatedao.value:{
            updatedao_data data = std::get<updatedao_data>(data_var);

            details_t::idx_t details(XDAO_INFO, XDAO_INFO.value);
            const auto detail = details.find(data.code.value);
            CHECKC(detail != details.end(), propose_err::RECORD_NOT_FOUND, "record not found");

            CHECKC(detail->creator == data.owner, propose_err::PERMISSION_DENIED, "only the creator can operate");
            CHECKC(!data.groupid.empty(), propose_err::PARAM_ERROR, "groupid can not be empty");
            CHECKC(!(data.symcode.empty() ^ data.symcontract.empty()), propose_err::PARAM_ERROR, "symcode and symcontract must be null or not null");

            if( !data.symcode.empty() ){
                accounts accountstable(name(data.symcontract), data.owner.value);
                const auto ac = accountstable.find(symbol_code(data.symcode).raw());
                CHECKC(ac != accountstable.end(),  propose_err::SYMBOL_ERROR, "symcode or symcontract not found");
            }

            break;
        }
        case proposal_action_type::setpropstg.value:{
            setpropstg_data data = std::get<setpropstg_data>(data_var);

            gov_t::idx_t govs(XDAO_GOV, XDAO_GOV.value);
            const auto gov = govs.find(data.daocode.value);
            CHECKC(gov != govs.end(), propose_err::RECORD_NOT_FOUND, "record not found");

            CHECKC(data.owner == gov->creator, propose_err::PERMISSION_DENIED, "only the creator can operate");

            if(!data.proposers.empty()){
                for( set<name>::iterator proposers_iter = data.proposers.begin(); proposers_iter != data.proposers.end(); proposers_iter++ ){
                    CHECKC( is_account(*proposers_iter) ,propose_err::ACCOUNT_NOT_EXITS, "account not exits:"+(*proposers_iter).to_string() );
                }
                auto proposers =  gov->proposers;
                proposers.insert(data.proposers.begin(), data.proposers.end());
            }

            if(!data.proposestg.empty()){
                strategy_t::idx_t stg(XDAO_STG, XDAO_STG.value);
                for( set<uint64_t>::iterator proposestg_iter = data.proposestg.begin(); proposestg_iter != data.proposestg.end(); proposestg_iter++ ){
                    CHECKC(stg.find(*proposestg_iter) != stg.end(), propose_err::STRATEGY_NOT_FOUND, "strategy not found:"+to_string(*proposestg_iter) );
                }
                auto propstg =  gov->propose_strategies;
                propstg.insert(data.proposestg.begin(), data.proposestg.end());
            }
            break;
        }
        default:{
            CHECKC(false, err::PARAM_ERROR, "Unsupport proposal type")
            break;
        }
    }
}

const xdaopropose::conf_t& xdaopropose::_conf() {
    if (!_conf_ptr) {
        _conf_tbl_ptr = make_unique<conf_table_t>(XDAO_CONF, XDAO_CONF.value);
        CHECKC(_conf_tbl_ptr->exists(), propose_err::SYSTEM_ERROR, "conf table not existed in contract" );
        _conf_ptr = make_unique<conf_t>(_conf_tbl_ptr->get());
    }
    return *_conf_ptr;
}