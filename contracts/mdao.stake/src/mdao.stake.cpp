#include <mdao.stake/mdao.stake.hpp>
#include <mdao.stake/mdao.stake.db.hpp>
#include <thirdparty/utils.hpp>
#include <thirdparty/safe.hpp>
#include <mdao.info/mdao.info.hpp>

// transfer out from contract self
#define TRANSFERFROM(bank, from, to, quantity, memo) \
    { action(permission_level{get_self(), "active"_n }, bank, "transfer"_n, std::make_tuple(from, to, quantity, memo )).send(); }
ACTION mdaostake::init( const set<name>& managers, const set<name>& supported_contracts ) {
    require_auth( _self );
    //CHECKC(!_gstate.initialized, stake_err::INITIALIZED, "already initialized")
    _gstate.managers = managers;
    _gstate.supported_contracts = supported_contracts;
    _gstate.initialized = true;
    // _global.set(_gstate, get_self());

    // dao_stake_t::idx_t ds( _self,_self.value);
    // auto itr = ds.begin();
    // while( itr != ds.end()){
    //     itr = ds.erase(itr);
    // }

    // user_stake_t::idx_t us( _self,_self.value);
    // auto d_itr = us.begin();
    // while(d_itr != us.end()){
    //     d_itr = us.erase(d_itr);
    // }
}

void mdaostake::staketoken(const name& from, const name& to, const asset& quantity, const string& memo )
{
    // CHECKC( false, stake_err::UNINITIALIZED, "contract uninitialized" );
    if(to != get_self()) return;
    CHECKC( _gstate.initialized, stake_err::UNINITIALIZED, "contract uninitialized" );
    CHECKC( quantity.amount>0, stake_err::NOT_POSITIVE, "swap quanity must be positive" )
    name daocode = name(memo);
    dao_info_t::idx_t info_tbl(MDAO_INFO, MDAO_INFO.value);
    const auto info = info_tbl.find(daocode.value);
    CHECKC( info != info_tbl.end(), stake_err::DAO_NOT_FOUND, "dao not exists");
    name contract = get_first_receiver();
    CHECKC( _gstate.supported_contracts.count(contract), stake_err::UNSUPPORT_CONTRACT, "unsupport token contract");
    // @todo dao, user check
    // find record at daostake table
    dao_stake_t dao_stake(daocode);
    if( !_db.get(dao_stake) ) {
        dao_stake.daocode = daocode;
        dao_stake.tokens_stake = map<extended_symbol, int64_t>();
        dao_stake.nfts_stake = map<extended_nsymbol, int64_t>();
        dao_stake.user_count = uint32_t(0);
    }
    // find record at userstake table
    user_stake_t::idx_t user_stake_table( get_self(), get_self().value);
    auto user_stake_index = user_stake_table.get_index<"unionid"_n>();
    auto user_stake_iter = user_stake_index.find(get_unionid(from,daocode));
    user_stake_t user_stake(daocode, from);
    if(user_stake_iter == user_stake_index.end()) {
        // record not found
        dao_stake.tokens_stake = map<extended_symbol, int64_t>();
        dao_stake.nfts_stake = map<extended_nsymbol, int64_t>();
        user_stake.freeze_until = time_point_sec(uint32_t(0));
        dao_stake.user_count ++;
        user_stake.id = user_stake_table.available_primary_key();
    } else {
        // record exist
        user_stake.id = user_stake_iter->id;
        CHECKC(_db.get(user_stake), stake_err::STAKE_NOT_FOUND, "no stake record");
    }
    
    extended_symbol sym = extended_symbol{quantity.symbol, contract};
    dao_stake.tokens_stake[sym] = 
        (safe<int64_t>(dao_stake.tokens_stake[sym]) + safe<int64_t>(quantity.amount)).value;
    user_stake.tokens_stake[sym] =
        (safe<int64_t>(user_stake.tokens_stake[sym]) + safe<int64_t>(quantity.amount)).value;
    // update database
    _db.set(user_stake, get_self());
    _db.set(dao_stake, get_self());
}

ACTION mdaostake::unstaketoken(const uint64_t &id, const vector<extended_asset> &tokens)
{
    CHECKC(_gstate.initialized, stake_err::UNINITIALIZED, "contract uninitialized");
    user_stake_t user_stake(id);
    CHECKC(_db.get(user_stake), stake_err::STAKE_NOT_FOUND,"no stake record");
    name account = user_stake.account;
    name daocode = user_stake.daocode;
    require_auth(account);
    // find record at daostake table
    dao_stake_t dao_stake(daocode);
    CHECKC(_db.get(dao_stake), stake_err::DAO_NOT_FOUND, "dao not found");

    CHECKC( time_point_sec(current_time_point())>user_stake.freeze_until, stake_err::STILL_IN_LOCK, "still in lock" )
    // iterate over the input and withdraw token
    vector<extended_asset>::const_iterator out_iter = tokens.begin();
    for (; out_iter!= tokens.end(); out_iter++) {
        extended_asset token = *out_iter;
        extended_symbol sym = token.get_extended_symbol();
        CHECKC(token.quantity.amount > 0 && token.quantity.is_valid(), stake_err::INVALID_PARAMS, "invalid amount");
        CHECKC(token.quantity.amount <= user_stake.tokens_stake[sym], stake_err::UNLOCK_OVERFLOW, "stake amount not enough");
        user_stake.tokens_stake[sym] =
            (safe<int64_t>(user_stake.tokens_stake[sym]) - safe<int64_t>(token.quantity.amount)).value;
        dao_stake.tokens_stake[sym] = 
            (safe<int64_t>(dao_stake.tokens_stake[sym]) - safe<int64_t>(token.quantity.amount)).value;
        if(user_stake.tokens_stake[sym]==0) {
            user_stake.tokens_stake.erase(sym);
            if (dao_stake.tokens_stake[sym] == 0)
            {
                dao_stake.tokens_stake.erase(sym);
            }
        }
        TRANSFERFROM(token.contract, get_self(), account, token, string("redeem transfer"));
    }
    // update database
    if(user_stake.tokens_stake.empty()&&user_stake.nfts_stake.empty()) {
        dao_stake.user_count --;
        _db.del(user_stake);
    } else {
        _db.set(user_stake, account);
    }
    _db.set(dao_stake, get_self());
}

void mdaostake::stakenft( name from, name to, vector< nasset >& assets, string memo )
{
    if(to != get_self()) return;
    CHECKC( _gstate.initialized, stake_err::UNINITIALIZED, "contract uninitialized" );
    // CHECKC( quantity.amount>0, stake_err::NOT_POSITIVE, "swap quanity must be positive" )
    name daocode = name(memo);
    dao_info_t::idx_t info_tbl(MDAO_INFO, MDAO_INFO.value);
    const auto info = info_tbl.find(daocode.value);
    CHECKC( info != info_tbl.end(), stake_err::DAO_NOT_FOUND, "dao not exists");
    name contract = get_first_receiver();
    CHECKC( _gstate.supported_contracts.count(contract), stake_err::UNSUPPORT_CONTRACT, "unsupport token contract");
    // @todo dao, user check
    // find record at daostake table
    dao_stake_t dao_stake(daocode);
    if( !_db.get(dao_stake) ) {
        dao_stake.daocode = daocode;
        dao_stake.tokens_stake = map<extended_symbol, int64_t>();
        dao_stake.nfts_stake = map<extended_nsymbol, int64_t>();
        dao_stake.user_count = uint32_t(0);
    }
    // find record at userstake table
    user_stake_t::idx_t user_stake_table( get_self(),  get_self().value);
    auto user_stake_index = user_stake_table.get_index<"unionid"_n>();
    auto user_stake_iter = user_stake_index.find(get_unionid(from,daocode));
    user_stake_t user_stake(daocode, from);
    if(user_stake_iter == user_stake_index.end()) {
        // record not found
        dao_stake.tokens_stake = map<extended_symbol, int64_t>();
        dao_stake.nfts_stake = map<extended_nsymbol, int64_t>();
        user_stake.freeze_until = time_point_sec(uint32_t(0));
        dao_stake.user_count ++;
        user_stake.id = user_stake_table.available_primary_key();
    } else {
        // record exist
        user_stake.id = user_stake_iter->id;
        CHECKC(_db.get(user_stake), stake_err::STAKE_NOT_FOUND, "no stake record");
    }
    // iterate over the input and stake nft
    vector<nasset>::const_iterator in_iter = assets.begin();
    for (; in_iter!= assets.end(); in_iter++) {
        nasset ntoken = *in_iter;
        extended_nsymbol sym = extended_nsymbol{ntoken.symbol,contract};
        CHECKC( ntoken.amount > 0, stake_err::INVALID_PARAMS, "stake amount invalid");
        dao_stake.nfts_stake[sym] =
            (safe<int64_t>(dao_stake.nfts_stake[sym]) + safe<int64_t>(ntoken.amount)).value;
        user_stake.nfts_stake[sym] =
            (safe<int64_t>(user_stake.nfts_stake[sym]) + safe<int64_t>(ntoken.amount)).value;
    }
    // update database
    _db.set(user_stake,  get_self());
    _db.set(dao_stake,  get_self());
}

ACTION mdaostake::unstakenft(const uint64_t &id, const vector<extended_nasset> &nfts)
{
    CHECKC(_gstate.initialized, stake_err::UNINITIALIZED, "contract uninitialized");
    user_stake_t user_stake(id);
    CHECKC(_db.get(user_stake), stake_err::STAKE_NOT_FOUND,"no stake record");
    name account = user_stake.account;
    name daocode = user_stake.daocode;
    require_auth(account);
    // find record at daostake table
    dao_stake_t dao_stake(daocode);
    CHECKC(_db.get(dao_stake), stake_err::DAO_NOT_FOUND, "dao not found");

    CHECKC(time_point_sec(current_time_point()) > user_stake.freeze_until, stake_err::STILL_IN_LOCK, "still in lock")
    // iterate over the input and withdraw nft
    vector<extended_nasset>::const_iterator out_iter = nfts.begin();
    for (; out_iter!= nfts.end(); out_iter++) {
        extended_nasset ntoken = *out_iter;
        extended_nsymbol sym = ntoken.get_extended_nsymbol();
        CHECKC( ntoken.quantity.amount > 0 && ntoken.quantity.is_valid(), stake_err::INVALID_PARAMS, "invalid amount");
        CHECKC( ntoken.quantity.amount <= user_stake.nfts_stake[sym], stake_err::UNLOCK_OVERFLOW, "stake amount not enough" );
        dao_stake.nfts_stake[sym] =
            (safe<int64_t>(dao_stake.nfts_stake[sym]) - safe<int64_t>(ntoken.quantity.amount)).value;
        user_stake.nfts_stake[sym] =
            (safe<int64_t>(user_stake.nfts_stake[sym]) - safe<int64_t>(ntoken.quantity.amount)).value;
        vector<nasset> nft = {ntoken.quantity};
        TRANSFERFROM(sym.get_contract(),get_self(), account, nft , string("redeem transfer"));
        if(user_stake.nfts_stake[sym]==0) {
            user_stake.nfts_stake.erase(sym);
            if (dao_stake.nfts_stake[sym] == 0)
            {
                dao_stake.nfts_stake.erase(sym);
            }
        }
    }
    // update database
    if (user_stake.tokens_stake.empty() && user_stake.nfts_stake.empty()) {
        dao_stake.user_count--;
        _db.del(user_stake);
    } else {
        _db.set(user_stake, account);
    }
    _db.set(dao_stake,  get_self());
}

ACTION mdaostake::extendlock(const name &manager, uint64_t &id, const uint32_t &locktime){
    require_auth( manager );
    CHECKC(_gstate.managers.count(manager)>0, stake_err::NO_PERMISSION, "no permission");
    // find record at userstake table
    user_stake_t user_stake(id);
    CHECKC(_db.get(user_stake), stake_err::STAKE_NOT_FOUND,"no stake record");
    time_point_sec new_unlockline = time_point_sec(current_time_point()) + locktime;
    user_stake.freeze_until = max(user_stake.freeze_until, new_unlockline);
    // update database
    _db.set(user_stake,  get_self());
}
