#include <mdao.stake/mdao.stake.hpp>
#include <mdao.stake/mdao.stake.db.hpp>
#include <thirdparty/utils.hpp>
#include <thirdparty/safe.hpp>

// transfer out from contract self
#define TRANSFER_OUT(bank, to, quantity, memo) \
    { action(permission_level{get_self(), "active"_n }, bank, "transfer"_n, std::make_tuple(get_self(), to, quantity, memo )).send(); }

// transfer out from contract self
#define TRANSFER_NFT_OUT(bank, to, quantity, memo) \
    { action(permission_level{get_self(), "active"_n }, bank, "transfer"_n, std::make_tuple(get_self(), to, quantity, memo )).send(); }

inline uint64_t max(uint64_t a, uint64_t b) { return a>b?a:b; } 

ACTION mdaostake::init( const name& manager, set<name>supported_contracts ) {
    require_auth( _self );
    CHECKC(!_gstate.initialized, stake_err::INITIALIZED, "already initialized")
    _gstate.manager = manager;
    _gstate.supported_contracts = supported_contracts;
    _gstate.initialized = true;
    _global.set(_gstate, get_self());
}

ACTION mdaostake::staketoken(const name& from, const name& to, const asset& quantity, const string& memo )
{
    if(to != get_self()) return;
    CHECKC( _gstate.initialized, stake_err::UNINITIALIZED, "contract uninitialized" );
    CHECKC( quantity.amount>0, stake_err::NOT_POSITIVE, "swap quanity must be positive" )
    name daocode = name(memo);
    name contract = get_first_receiver();
    CHECKC( _gstate.supported_contracts.count(contract), stake_err::UNSUPPORT_CONTRACT, "unsupport token contract");
    // @todo dao, user check
    // find record at daostake table
    dao_stake_t dao_stake(daocode);
    if( !_db.get(dao_stake) ) {
        dao_stake.daocode = daocode;
        dao_stake.token_stake = map<symbol, uint64_t>();
        dao_stake.nft_stake = map<uint64_t, uint64_t>();
        dao_stake.user_count = uint64_t(0);
    }
    // find record at userstake table
    user_stake_t::idx_t user_stake_table( get_self(), get_self().value);
    auto user_stake_index = user_stake_table.get_index<"unionid"_n>();
    auto user_stake_iter = user_stake_index.find(get_unionid(from,daocode));
    user_stake_t user_stake(daocode, from);
    if(user_stake_iter == user_stake_index.end()) {
        // record not fount
        user_stake.token_stake = map<symbol, uint64_t>();
        user_stake.nft_stake = map<uint64_t, uint64_t>();
        user_stake.freeze_until = time_point_sec(uint32_t(0));
        dao_stake.user_count ++;
    } else {
        // record exsit
        user_stake.id = user_stake_iter->id;
        CHECKC(_db.get(user_stake), stake_err::STAKE_NOT_FOUND, "no stake record");
    }
    // TRANSFER( account,self,amount,"stake" );
    dao_stake.token_stake[quantity.symbol] = 
        (safe<uint64_t>(dao_stake.token_stake[quantity.symbol]) + safe<uint64_t>(quantity.amount)).value;
    user_stake.token_stake[quantity.symbol] =
        (safe<uint64_t>(user_stake.token_stake[quantity.symbol]) + safe<uint64_t>(quantity.amount)).value;
    // update database
    _db.set(user_stake, get_self());
    _db.set(dao_stake, get_self());
}

ACTION mdaostake::unlocktoken(const name &account, const name &daocode, const vector<asset> &tokens)
{
    require_auth(account);
    CHECKC(_gstate.initialized, stake_err::UNINITIALIZED, "contract uninitialized");
    // find record at daostake table
    dao_stake_t dao_stake(daocode);
    CHECKC(_db.get(dao_stake), stake_err::DAO_NOT_FOUND, "dao not found");
    // find record at userstake table
    user_stake_t::idx_t user_stake_table( get_self(),  get_self().value);
    auto user_stake_index = user_stake_table.get_index<"unionid"_n>();
    auto user_stake_iter = user_stake_index.find(get_unionid(account,daocode));
    CHECKC(user_stake_iter != user_stake_index.end(), stake_err::STAKE_NOT_FOUND, "no stake record");
    user_stake_t user_stake(user_stake_iter->id, daocode, account);
    CHECKC(_db.get(user_stake), stake_err::STAKE_NOT_FOUND,"no stake record");

    CHECKC( time_point_sec(current_time_point())>user_stake.freeze_until, stake_err::STILL_IN_LOCK, "still in lock" )
    // iterate over the input and withdraw token
    vector<asset>::const_iterator out_iter = tokens.begin();
    for (; out_iter!= tokens.end(); out_iter++) {
        asset token = *out_iter;
        CHECKC(token.amount > 0 && token.is_valid(), stake_err::INVALID_PARAMS, "invalid amount");
        CHECKC( token.amount<=user_stake.token_stake[token.symbol], stake_err::UNLOCK_OVERFLOW, "stake amount not enough" );
        // TRANSFER(self, account, amount, "stake");
        user_stake.token_stake[token.symbol] =
            (safe<uint64_t>(dao_stake.token_stake[token.symbol]) - safe<uint64_t>(token.amount)).value;;
        dao_stake.token_stake[token.symbol] = 
            (safe<uint64_t>(dao_stake.token_stake[token.symbol]) - safe<uint64_t>(token.amount)).value;;
        if(user_stake.token_stake[token.symbol]==0) {
            user_stake.token_stake.erase(token.symbol);
            if (dao_stake.token_stake[token.symbol] == 0)
            {
                dao_stake.token_stake.erase(token.symbol);
            }
        }
        TRANSFER_OUT(get_self(), account, token, string("redeem transfer"));
    }
    // update database
    _db.set(user_stake, get_self());
    _db.set(dao_stake, get_self());
}

ACTION mdaostake::stakenft( name from, name to, vector< nasset >& assets, string memo )
{
    if(to != get_self()) return;
    CHECKC( _gstate.initialized, stake_err::UNINITIALIZED, "contract uninitialized" );
    // CHECKC( quantity.amount>0, stake_err::NOT_POSITIVE, "swap quanity must be positive" )
    name daocode = name(memo);
    name contract = get_first_receiver();
    CHECKC( _gstate.supported_contracts.count(contract), stake_err::UNSUPPORT_CONTRACT, "unsupport token contract");
    // @todo dao, user check
    // find record at daostake table
    dao_stake_t dao_stake(daocode);
    if( !_db.get(dao_stake) ) {
        dao_stake.daocode = daocode;
        dao_stake.token_stake = map<symbol, uint64_t>();
        dao_stake.nft_stake = map<uint64_t, uint64_t>();
        dao_stake.user_count = uint64_t(0);
    }
    // find record at userstake table
    user_stake_t::idx_t user_stake_table( get_self(),  get_self().value);
    auto user_stake_index = user_stake_table.get_index<"unionid"_n>();
    auto user_stake_iter = user_stake_index.find(get_unionid(from,daocode));
    user_stake_t user_stake(daocode, from);
    if(user_stake_iter == user_stake_index.end()) {
        // record not fount
        user_stake.token_stake = map<symbol, uint64_t>();
        user_stake.nft_stake = map<uint64_t, uint64_t>();
        user_stake.freeze_until = time_point_sec(uint32_t(0));
        dao_stake.user_count ++;
    } else {
        // record exsit
        user_stake.id = user_stake_iter->id;
        CHECKC(_db.get(user_stake), stake_err::STAKE_NOT_FOUND, "no stake record");
    }
    // iterate over the input and stake nft
    vector<nasset>::const_iterator in_iter = assets.begin();
    for (; in_iter!= assets.end(); in_iter++) {
        nasset ntoken = *in_iter;
        CHECKC( ntoken.amount > 0, stake_err::INVALID_PARAMS, "stake amount invalid");
        // TRANSFER_N( account,self,amount,"stake" );
        dao_stake.nft_stake[ntoken.symbol.raw()] =
            (safe<uint64_t>(dao_stake.nft_stake[ntoken.symbol.raw()]) + safe<uint64_t>(ntoken.amount)).value;
        user_stake.nft_stake[ntoken.symbol.raw()] =
            (safe<uint64_t>(user_stake.nft_stake[ntoken.symbol.raw()]) + safe<uint64_t>(ntoken.amount)).value;
    }
    // update database
    _db.set(user_stake,  get_self());
    _db.set(dao_stake,  get_self());
}

ACTION mdaostake::unlocknft(const name &account, const name &daocode, const vector<nasset> &nfts)
{
    require_auth(account);
    CHECKC(_gstate.initialized, stake_err::UNINITIALIZED, "contract uninitialized");
    // find record at daostake table
    dao_stake_t dao_stake(daocode);
    CHECKC(_db.get(dao_stake), stake_err::DAO_NOT_FOUND, "dao not found");
    // find record at userstake table
    user_stake_t::idx_t user_stake_table( get_self(),  get_self().value);
    auto user_stake_index = user_stake_table.get_index<"unionid"_n>();
    auto user_stake_iter = user_stake_index.find(get_unionid(account,daocode));
    CHECKC(user_stake_iter != user_stake_index.end(), stake_err::STAKE_NOT_FOUND, "no stake record");
    user_stake_t user_stake(user_stake_iter->id, daocode, account);
    CHECKC(_db.get(user_stake), stake_err::STAKE_NOT_FOUND, "no stake record");

    CHECKC(time_point_sec(current_time_point()) > user_stake.freeze_until, stake_err::STILL_IN_LOCK, "still in lock")
    // iterate over the input and withdraw nft
    vector<nasset>::const_iterator out_iter = nfts.begin();
    for (; out_iter!= nfts.end(); out_iter++) {
        nasset ntoken = *out_iter;
        CHECKC( ntoken.amount > 0 && ntoken.is_valid(), stake_err::INVALID_PARAMS, "invalid amount");
        CHECKC( ntoken.amount<=user_stake.nft_stake[ntoken.symbol.raw()], stake_err::UNLOCK_OVERFLOW, "stake amount not enough" );
        // TRANSFER(self, account, amount, "stake");
        dao_stake.nft_stake[ntoken.symbol.raw()] =
            (safe<uint64_t>(dao_stake.nft_stake[ntoken.symbol.raw()]) - safe<uint64_t>(ntoken.amount)).value;
        user_stake.nft_stake[ntoken.symbol.raw()] =
            (safe<uint64_t>(user_stake.nft_stake[ntoken.symbol.raw()]) - safe<uint64_t>(ntoken.amount)).value;
        TRANSFER_NFT_OUT(get_self(), account, ntoken, string("redeem transfer"));
        if(user_stake.nft_stake[ntoken.symbol.raw()]==0) {
            user_stake.nft_stake.erase(ntoken.symbol.raw());
            if (dao_stake.nft_stake[ntoken.symbol.raw()] == 0)
            {
                dao_stake.nft_stake.erase(ntoken.symbol.raw());
            }
        }
    }
    // update database
    _db.set(user_stake,  get_self());
    _db.set(dao_stake,  get_self());
}

ACTION mdaostake::extendlock(const name &account, const name &daocode, const uint64_t &locktime){
    require_auth( _gstate.manager );
    // find record at userstake table
    user_stake_t::idx_t user_stake_table( get_self(),  get_self().value);
    auto user_stake_index = user_stake_table.get_index<"unionid"_n>();
    auto user_stake_iter = user_stake_index.find(get_unionid(account,daocode));
    CHECKC(user_stake_iter != user_stake_index.end(), stake_err::STAKE_NOT_FOUND, "no stake record");
    user_stake_t user_stake(user_stake_iter->id, daocode, account);
    CHECKC(_db.get(user_stake), stake_err::STAKE_NOT_FOUND, "no stake record");
    time_point_sec new_unlockline = time_point_sec(current_time_point()) + locktime;
    user_stake.freeze_until = max(user_stake.freeze_until, new_unlockline);
    // update database
    _db.set(user_stake,  get_self());
}