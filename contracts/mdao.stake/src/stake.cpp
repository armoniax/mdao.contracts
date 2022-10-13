#include <mdao.stake/mdao.stake.hpp>
#include <mdao.stake/mdao.stake.db.hpp>
#include <thirdparty/utils.hpp>

ACTION mdaostake::staketoken(const name &account, const name &daocode, const vector<asset> &tokens, const uint64_t &locktime)
{
    require_auth( account );
    uint64_t id = auto_hash_key(account, daocode);
    time_point_sec new_unlockline = time_point_sec(current_time_point()) + locktime;
    // @todo dao, user check
    // find record at daostake table
    auto dao_stake_iter = daostaketable.find(daocode.value);
    if( dao_stake_iter == daostaketable.end() ) {
        dao_stake_iter = daostaketable.emplace(_self, [&](auto& u){
            u.daocode = daocode;
            u.token_stake = map<symbol, uint64_t>();
            u.user_count = uint64_t(0);
        });
    }
    uint64_t user_count =  dao_stake_iter -> user_count;
    // find record at userstake table
    auto user_stake_iter = userstaketable.find(id);
    if( user_stake_iter == userstaketable.end() ) {
        // user not found
        user_stake_iter = userstaketable.emplace(_self, [&](auto& u) {
            u.id = id;
            u.account = account;
            u.daocode = daocode;
            u.token_stake = map<symbol, uint64_t>();
            u.nft_stake = map<uint64_t, uint64_t>();
            u.freeze_until = time_point_sec(uint32_t(0));
        });
        // new user
        user_count ++;
    } else {
        CHECK(account.value==user_stake_iter->account.value, "conflict account");
        CHECK(daocode.value==user_stake_iter->daocode.value, "conflict daocode");
    }
    // copy token stake map
    map<symbol, uint64_t> new_dao_staked_token = dao_stake_iter->token_stake;
    map<symbol, uint64_t> new_user_staked_token = user_stake_iter->token_stake;
    // iterate over the input and stake token
    vector<asset>::const_iterator in_iter = tokens.begin();
    for (; in_iter!= tokens.end(); in_iter++) {
        asset token = *in_iter;
        CHECK( (token.is_valid() && token.amount>0), "stake amount invalid" );
        // TRANSFER( account,self,amount,"stake" );
        new_user_staked_token[token.symbol] += token.amount;
        new_dao_staked_token[token.symbol] += token.amount;
    }
    if(user_stake_iter->freeze_until>new_unlockline) {
        new_unlockline = user_stake_iter->freeze_until;
    }
    // update database
    userstaketable.modify(user_stake_iter,_self,[&](auto& u) {
        u.token_stake = new_user_staked_token;
        u.freeze_until = new_unlockline;
    });
    daostaketable.modify(dao_stake_iter,_self,[&](auto& u) {
        u.token_stake = new_dao_staked_token;
        u.user_count = user_count;
    });
}

ACTION mdaostake::unlocktoken(const name &account, const name &daocode, const vector<asset> &tokens)
{
    require_auth( account );
    uint64_t id = auto_hash_key(account, daocode);
    // find record at daostake table
    auto dao_stake_iter = daostaketable.find(daocode.value);
    CHECK(dao_stake_iter != daostaketable.end(), "dao not found");
    // find record at userstake table
    auto user_stake_iter = userstaketable.find(id);
    CHECK(user_stake_iter != userstaketable.end(), "no stake record");
    CHECK(account.value == user_stake_iter->account.value, "conflict account");
    CHECK(daocode.value == user_stake_iter->daocode.value, "conflict daocode");

    CHECK( time_point_sec(current_time_point())>user_stake_iter->freeze_until, "still in lock" )
    // copy token stake map
    map<symbol, uint64_t> new_dao_staked_token = dao_stake_iter->token_stake;
    map<symbol, uint64_t> new_user_staked_token = user_stake_iter->token_stake;
    // iterate over the input and withdraw token
    vector<asset>::const_iterator out_iter = tokens.begin();
    for (; out_iter!= tokens.end(); out_iter++) {
        asset token = *out_iter;
        CHECK( token.amount<=new_user_staked_token[token.symbol], "stake amount not enough" );
        // TRANSFER(self, account, amount, "stake");
        new_user_staked_token[token.symbol] -= token.amount;
        new_dao_staked_token[token.symbol] -= token.amount;
        if(new_user_staked_token[token.symbol]==0) {
            new_user_staked_token.erase(token.symbol);
            if (new_dao_staked_token[token.symbol] == 0)
            {
                new_dao_staked_token.erase(token.symbol);
            }
        }
    }
    // update database
    userstaketable.modify(user_stake_iter,_self,[&](auto& u) {
        u.token_stake = new_user_staked_token;
        // u.freeze_until = 0;
    });
    daostaketable.modify(dao_stake_iter,_self,[&](auto& u) {
        u.token_stake = new_dao_staked_token;
    });
}

ACTION mdaostake::stakenft(const name &account, const name &daocode, const vector<nasset> &nfts, const uint64_t &locktime)
{
    require_auth( account );
    uint64_t id = auto_hash_key(account, daocode);
    time_point_sec new_unlockline = time_point_sec(current_time_point()) + locktime;
    // @todo dao, user check
    // find record at daostake table
    auto dao_stake_iter = daostaketable.find(daocode.value);
    if( dao_stake_iter == daostaketable.end() ) {
        dao_stake_iter = daostaketable.emplace(_self, [&](auto& u){
            u.daocode = daocode;
            u.token_stake = map<symbol, uint64_t>();
            u.nft_stake = map<uint64_t, uint64_t>();
            u.user_count = uint64_t(0);
        });
    }
    uint64_t user_count =  dao_stake_iter -> user_count;
    // find record at userstake table
    auto user_stake_iter = userstaketable.find(id);
    if( user_stake_iter == userstaketable.end() ) {
        // user not found
        user_stake_iter = userstaketable.emplace(_self, [&](auto& u) {
            u.id = id;
            u.account = account;
            u.daocode = daocode;
            u.token_stake = map<symbol, uint64_t>();
            u.nft_stake = map<uint64_t, uint64_t>();
            u.freeze_until = time_point_sec(uint32_t(0));
        });
        // new user
        user_count ++;
    } else {
        CHECK(account.value==user_stake_iter->account.value, "conflict account");
        CHECK(daocode.value==user_stake_iter->daocode.value, "conflict daocode");
    }
    // copy nft stake map
    map<uint64_t, uint64_t> new_dao_staked_nft = dao_stake_iter->nft_stake;
    map<uint64_t, uint64_t> new_user_staked_nft = user_stake_iter->nft_stake;
    // iterate over the input and stake nft
    vector<nasset>::const_iterator in_iter = nfts.begin();
    for (; in_iter!= nfts.end(); in_iter++) {
        nasset ntoken = *in_iter;
        CHECK((ntoken.is_valid() && ntoken.amount > 0), "stake amount invalid");
        // TRANSFER_N( account,self,amount,"stake" );
        new_user_staked_nft[ntoken.symbol.raw()] += ntoken.amount;
        new_dao_staked_nft[ntoken.symbol.raw()] += ntoken.amount;
    }
    if(user_stake_iter->freeze_until>new_unlockline) {
        new_unlockline = user_stake_iter->freeze_until;
    }
    // update database
    userstaketable.modify(user_stake_iter,_self,[&](auto& u) {
        u.nft_stake = new_user_staked_nft;
        u.freeze_until = new_unlockline;
    });
    daostaketable.modify(dao_stake_iter,_self,[&](auto& u) {
        u.nft_stake = new_dao_staked_nft;
        u.user_count = user_count;
    });
}

ACTION mdaostake::unlocknft(const name &account, const name &daocode, const vector<nasset> &nfts)
{
    require_auth( account );
    uint64_t id = auto_hash_key(account, daocode);
    // find record at daostake table
    auto dao_stake_iter = daostaketable.find(daocode.value);
    CHECK(dao_stake_iter != daostaketable.end(), "dao not found");
    // find record at userstake table
    auto user_stake_iter = userstaketable.find(id);
    CHECK(user_stake_iter != userstaketable.end(), "no stake record");
    CHECK(account.value == user_stake_iter->account.value, "conflict account");
    CHECK(daocode.value == user_stake_iter->daocode.value, "conflict daocode");

    CHECK( time_point_sec(current_time_point())>user_stake_iter->freeze_until, "still in lock" )
    // copy nft stake map
    map<uint64_t, uint64_t> new_dao_staked_nft = dao_stake_iter->nft_stake;
    map<uint64_t, uint64_t> new_user_staked_nft = user_stake_iter->nft_stake;
    // iterate over the input and withdraw nft
    vector<nasset>::const_iterator out_iter = nfts.begin();
    for (; out_iter!= nfts.end(); out_iter++) {
        nasset ntoken = *out_iter;
        CHECK( ntoken.amount<=new_user_staked_nft[ntoken.symbol.raw()], "stake amount not enough" );
        // TRANSFER(self, account, amount, "stake");
        new_user_staked_nft[ntoken.symbol.raw()] -= ntoken.amount;
        new_dao_staked_nft[ntoken.symbol.raw()] -= ntoken.amount;
        if(new_user_staked_nft[ntoken.symbol.raw()]==0) {
            new_user_staked_nft.erase(ntoken.symbol.raw());
            if (new_dao_staked_nft[ntoken.symbol.raw()] == 0)
            {
                new_dao_staked_nft.erase(ntoken.symbol.raw());
            }
        }
    }
    // update database
    userstaketable.modify(user_stake_iter,_self,[&](auto& u) {
        u.nft_stake = new_user_staked_nft;
        // u.freeze_until = 0;
    });
    daostaketable.modify(dao_stake_iter,_self,[&](auto& u) {
        u.nft_stake = new_dao_staked_nft;
    });
}
