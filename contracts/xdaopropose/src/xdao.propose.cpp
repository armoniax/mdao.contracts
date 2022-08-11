#include <xdao.propose/xdao.propose.hpp>
#include <xdaostg/xdaostg.hpp>
#include <thirdparty/utils.hpp>
#include <set>

ACTION xdaopropose::create(const name& creator,const string& name, const string& desc,
                            const uint64_t& stgid, const uint32_t& votes)
{
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
    _db.set(propose);
}

ACTION xdaopropose::addoption( const name& owner, const uint64_t& proposeid, const string& title )
{
    require_auth( owner );

    auto conf = _conf();
    CHECKC( conf.status != conf_status::MAINTAIN, propose_err::NOT_AVAILABLE, "under maintenance" );

    propose_t propose(proposeid);
    CHECKC( _db.get(propose) ,propose_err::RECORD_NOT_FOUND, "record not found" );
    CHECKC( owner == propose.creator, propose_err::PERMISSION_DENIED, "only the creator can operate" );
 
    option opt;
    opt.title        =   title;
    propose.opts.push_back(opt);
    _db.set(propose);

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
    _db.set(propose);

}

ACTION xdaopropose::excute(const name& owner, const uint64_t& proposeid)
{
    auto conf = _conf();
    CHECKC( conf.status != conf_status::MAINTAIN, propose_err::NOT_AVAILABLE, "under maintenance" );

    propose_t propose(proposeid);
    CHECKC( _db.get(propose) ,propose_err::RECORD_NOT_FOUND, "record not found" );
    CHECKC( owner == propose.creator, propose_err::PERMISSION_DENIED, "only the creator can operate" );
    CHECKC( propose.status == propose_status::RUNNING, propose_err::STATUS_ERROR, "propose status must be running" );
    for( vector<option>::iterator opt_iter = propose.opts.begin(); opt_iter != propose.opts.end(); opt_iter++ ){
        CHECKC( (*opt_iter).recv_votes >= propose.req_votes, propose_err::INSUFFICIENT_VOTES, "insufficient votes" );
    }
    propose.status  =  propose_status::EXCUTING;
    _db.set(propose);
}

ACTION xdaopropose::votefor(const name& voter, const uint64_t& proposeid)
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
        (*opt_iter).recv_votes  +=  stg_weight;
    }

    _db.set(propose);
}

const xdaopropose::conf_t& xdaopropose::_conf() {
    if (!_conf_ptr) {
        _conf_tbl_ptr = make_unique<conf_table_t>(XDAO_CONF, XDAO_CONF.value);
        CHECKC(_conf_tbl_ptr->exists(), propose_err::SYSTEM_ERROR, "conf table not existed in contract" );
        _conf_ptr = make_unique<conf_t>(_conf_tbl_ptr->get());
    }
    return *_conf_ptr;
}