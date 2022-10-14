#include <mdao.stake/mdao.stake.hpp>
#include <mdao.stake/mdao.stake.db.hpp>
#include <thirdparty/utils.hpp>

ACTION mdaostake::staketoken(const name &account, const name &daocode, const vector<asset> &tokens, const uint64_t &locktime)
{
    require_auth( account );
    time_point_sec new_unlockline = time_point_sec(current_time_point()) + locktime;
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
    user_stake_t::idx_t user_stake_table(_self, _self.value);
    auto user_stake_index = user_stake_table.get_index<"unionid"_n>();
    auto user_stake_iter = user_stake_index.find(get_unionid(account,daocode));
    user_stake_t user_stake(daocode, account);
    if(user_stake_iter == user_stake_index.end()) {
        // record not fount
        user_stake.token_stake = map<symbol, uint64_t>();
        user_stake.nft_stake = map<uint64_t, uint64_t>();
        user_stake.freeze_until = time_point_sec(uint32_t(0));
        dao_stake.user_count ++;
    } else {
        // record exsit
        user_stake.id = user_stake_iter->id;
        CHECK(_db.get(user_stake), "no stake record");
    }
    // iterate over the input and stake token
    vector<asset>::const_iterator in_iter = tokens.begin();
    for (; in_iter!= tokens.end(); in_iter++) {
        asset token = *in_iter;
        CHECK( (token.is_valid() && token.amount>0), "stake amount invalid" );
        // TRANSFER( account,self,amount,"stake" );
        dao_stake.token_stake[token.symbol] += token.amount;
        user_stake.token_stake[token.symbol] += token.amount;
    }
    if(user_stake.freeze_until<new_unlockline) {
        user_stake.freeze_until = new_unlockline;
    }
    // update database
    _db.set(user_stake, _self);
    _db.set(dao_stake, _self);
}

ACTION mdaostake::unlocktoken(const name &account, const name &daocode, const vector<asset> &tokens)
{
    require_auth( account );
    // find record at daostake table
    dao_stake_t dao_stake(daocode);
    CHECK(_db.get(dao_stake), "dao not found");
    // find record at userstake table
    user_stake_t::idx_t user_stake_table(_self, _self.value);
    auto user_stake_index = user_stake_table.get_index<"unionid"_n>();
    auto user_stake_iter = user_stake_index.find(get_unionid(account,daocode));
    CHECK(user_stake_iter != user_stake_index.end(), "no stake record");
    user_stake_t user_stake(user_stake_iter->id, daocode, account);
    CHECK(_db.get(user_stake), "no stake record");

    CHECK( time_point_sec(current_time_point())>user_stake.freeze_until, "still in lock" )
    // iterate over the input and withdraw token
    vector<asset>::const_iterator out_iter = tokens.begin();
    for (; out_iter!= tokens.end(); out_iter++) {
        asset token = *out_iter;
        CHECK( token.amount<=user_stake.token_stake[token.symbol], "stake amount not enough" );
        // TRANSFER(self, account, amount, "stake");
        user_stake.token_stake[token.symbol] -= token.amount;
        dao_stake.token_stake[token.symbol] -= token.amount;
        if(user_stake.token_stake[token.symbol]==0) {
            user_stake.token_stake.erase(token.symbol);
            if (dao_stake.token_stake[token.symbol] == 0)
            {
                dao_stake.token_stake.erase(token.symbol);
            }
        }
    }
    // update database
    _db.set(user_stake, _self);
    _db.set(dao_stake, _self);
}

ACTION mdaostake::stakenft(const name &account, const name &daocode, const vector<nasset> &nfts, const uint64_t &locktime)
{
    require_auth( account );
    time_point_sec new_unlockline = time_point_sec(current_time_point()) + locktime;
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
    user_stake_t::idx_t user_stake_table(_self, _self.value);
    auto user_stake_index = user_stake_table.get_index<"unionid"_n>();
    auto user_stake_iter = user_stake_index.find(get_unionid(account,daocode));
    user_stake_t user_stake(daocode, account);
    if(user_stake_iter == user_stake_index.end()) {
        // record not fount
        user_stake.token_stake = map<symbol, uint64_t>();
        user_stake.nft_stake = map<uint64_t, uint64_t>();
        user_stake.freeze_until = time_point_sec(uint32_t(0));
        dao_stake.user_count ++;
    } else {
        // record exsit
        user_stake.id = user_stake_iter->id;
        CHECK(_db.get(user_stake), "no stake record");
    }
    // iterate over the input and stake nft
    vector<nasset>::const_iterator in_iter = nfts.begin();
    for (; in_iter!= nfts.end(); in_iter++) {
        nasset ntoken = *in_iter;
        CHECK((ntoken.is_valid() && ntoken.amount > 0), "stake amount invalid");
        // TRANSFER_N( account,self,amount,"stake" );
        dao_stake.nft_stake[ntoken.symbol.raw()] += ntoken.amount;
        user_stake.nft_stake[ntoken.symbol.raw()] += ntoken.amount;
    }
    if (user_stake.freeze_until < new_unlockline)
    {
        user_stake.freeze_until = new_unlockline;
    }
    // update database
    _db.set(user_stake, _self);
    _db.set(dao_stake, _self);
}

ACTION mdaostake::unlocknft(const name &account, const name &daocode, const vector<nasset> &nfts)
{
    require_auth( account );
    // find record at daostake table
    dao_stake_t dao_stake(daocode);
    CHECK(_db.get(dao_stake), "dao not found");
    // find record at userstake table
    user_stake_t::idx_t user_stake_table(_self, _self.value);
    auto user_stake_index = user_stake_table.get_index<"unionid"_n>();
    auto user_stake_iter = user_stake_index.find(get_unionid(account,daocode));
    CHECK(user_stake_iter != user_stake_index.end(), "no stake record");
    user_stake_t user_stake(user_stake_iter->id, daocode, account);
    CHECK(_db.get(user_stake), "no stake record");

    CHECK(time_point_sec(current_time_point()) > user_stake.freeze_until, "still in lock")
    // iterate over the input and withdraw nft
    vector<nasset>::const_iterator out_iter = nfts.begin();
    for (; out_iter!= nfts.end(); out_iter++) {
        nasset ntoken = *out_iter;
        CHECK( ntoken.amount<=user_stake.nft_stake[ntoken.symbol.raw()], "stake amount not enough" );
        // TRANSFER(self, account, amount, "stake");
        user_stake.nft_stake[ntoken.symbol.raw()] -= ntoken.amount;
        dao_stake.nft_stake[ntoken.symbol.raw()] -= ntoken.amount;
        if(user_stake.nft_stake[ntoken.symbol.raw()]==0) {
            user_stake.nft_stake.erase(ntoken.symbol.raw());
            if (dao_stake.nft_stake[ntoken.symbol.raw()] == 0)
            {
                dao_stake.nft_stake.erase(ntoken.symbol.raw());
            }
        }
    }
    // update database
    _db.set(user_stake, _self);
    _db.set(dao_stake, _self);
}

ACTION mdaostake::extendlock(const name &account, const name &daocode, const uint64_t &locktime){
    require_auth( account );
    // find record at userstake table
    user_stake_t::idx_t user_stake_table(_self, _self.value);
    auto user_stake_index = user_stake_table.get_index<"unionid"_n>();
    auto user_stake_iter = user_stake_index.find(get_unionid(account,daocode));
    CHECK(user_stake_iter != user_stake_index.end(), "no stake record");
    user_stake_t user_stake(user_stake_iter->id, daocode, account);
    CHECK(_db.get(user_stake), "no stake record");
    time_point_sec new_unlockline = time_point_sec(current_time_point()) + locktime;
    if(user_stake.freeze_until>new_unlockline) {
        new_unlockline = user_stake.freeze_until;
    }
    // update database
    _db.set(user_stake, _self);
}