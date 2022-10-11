#include <mdao.stake/mdao.stake.hpp>
#include <mdao.stake/mdao.stake.db.hpp>
#include <thirdparty/utils.hpp>
#include <eosio.token/eosio.token.hpp>
#include <map>

ACTION mdaostake::stakeToken(const name &account, const name &daocode, const map<name, asset> &tokens, const uint64_t &locktime)
{
    require_auth( account );
    user_stake_t usrstake( account, daocode );
    stake_t sysstake( daocode );
    map<name, asset>::const_iterator iter = tokens.begin();
    auto self = get_self();
    for (; iter!= tokens.end(); iter++) {
        auto token = iter->first;
        auto amount = iter->second;
        CHECK( amount.is_valid(), "stake amount invalid" );
        // TRANSFER( account,self,amount,"stake" );
        usrstake.token_stake[token] += amount;
        sysstake.token_stake[token] += amount;
    }
    auto new_unlockline = time_point_sec(current_time_point()) + locktime;
    if(usrstake.freeze_until<new_unlockline) {
        usrstake.freeze_until = new_unlockline;
    }
    _db.set(usrstake, self);
    _db.set(sysstake, self);
}

ACTION mdaostake::withdrawToken(const name &account, const name &daocode, const map<name, asset> &tokens)
{
    require_auth( account );
    user_stake_t usrstake( account, daocode );
    stake_t sysstake( daocode );
    auto self = get_self();
    
    CHECK( time_point_sec(current_time_point())>usrstake.freeze_until, "still in lock" )

    map<name, asset>::const_iterator iter = tokens.begin();

    for (; iter!= tokens.end(); iter++) {
        auto token = iter->first;
        auto amount = iter->second;
        CHECK( amount<=usrstake.token_stake[token], "stake amount not enough" );
        // TRANSFER(self, account, amount, "stake");
        usrstake.token_stake[token] -= amount;
        sysstake.token_stake[token] -= amount;
        if(usrstake.token_stake[token].amount==0) {
            usrstake.token_stake.erase(token);
            if (sysstake.token_stake[token].amount == 0)
            {
                sysstake.token_stake.erase(token);
            }
        }
    }
    // if(usrstake.nft_stake.count()==0) {
    //     // maybe delete usr
    // }

    _db.set(usrstake, self);
    _db.set(sysstake, self);
}

// ACTION mdaostake::stakeNft(const name &account, const name &daocode, const map<name, uint64_t> &nfts){
//     require_auth( account );
//     // TODO
//     CHECKC( nft exist ?, stake_err::UNDEFINED, "nft not exist" );
//     // TODO
//     CHECKC( has token balance ?, stake_err::UNDEFINED, "balance too low" );
//     // TODO
//     for nft in nfts {
//         stake
//     }
// }

// ACTION mdaostake::withdrawNft(const name &account, const name &daocode, const map<name, uint64_t> &nfts){
//     require_auth( account );
//     // TODO
//     CHECKC( freeze deadline expire ?, stake_err::UNDEFINED, "still in lock" );
//     // TODO
//     CHECKC( has lock balance ?, stake_err::UNDEFINED, "no stake balance" );
//     // TODO
//     for nft in nfts {
//         withdraw
//     }
// }

