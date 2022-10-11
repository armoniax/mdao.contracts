#include <mdao.stake/mdao.stake.hpp>
#include <mdao.stake/mdao.stake.db.hpp>
#include <map>

using namespace eosio;

ACTION mdaostake::stakeToken(const name &account, const name &daocode, const map<name, asset> &tokens, uint64_t locktime)
{
    require_auth( account );
    user_stake_t usrstake( account, daocode );
    stake_t sysstake( daocode );
    map<name, uint64_t>::iterator iter = tokens.begin();

    for (; iter!= tokens.end(); iter++) {
        auto token = iter->first;
        auto amount = iter->second;
        CHECKC( amount.is_valid(), stake_err::UNDEFINED, "stake amount invalid" )
        transfer( account,get_self(),amount,"stake" )
        usrstake.token_stake[token] += amount;
        sysstake.token_stake[token] += amount;
    }
    auto new_unlockline = time_point_sec(current_time_point()) + locktime;
    if(usrstake.freeze_until<new_unlockline) {
        usrstake.freeze_until = new_unlockline
    }
    _db.set(usrstake, get_self());
    _db.set(sysstake, get_self());
}

ACTION mdaostake::withdrawToken(const name &account, const name &daocode, const map<name, uint64_t> &tokens){
    require_auth( account );
    user_stake_t usrstake( account, daocode );
    stake_t sysstake( daocode );
    
    CHECKC( time_point_sec(current_time_point())>usrstake.freeze_until, stake_err::UNDEFINED, "still in lock" )

    map<name, uint64_t>::iterator iter = tokens.begin();

    for (; iter!= tokens.end(); iter++) {
        auto token = iter->first;
        auto amount = iter->second;
        CHECKC( amount<=usrstake.token_stake[token], stake_err::UNDEFINED, "stake amount not enough" )
        transfer(get_self(), account, amount, "stake");
        usrstake.token_stake[token] -= amount;
        sysstake.token_stake[token] -= amount;
        if(usrstake.token_stake[token]==0) {
            usrstake.token_stake.erase(token);
            if(sysstake.token_stake[token]==0) {
                sysstake.token_stake.erase(token);
            }
        }
    }
    // if(usrstake.nft_stake.count()==0) {
    //     // maybe delete usr
    // }

    _db.set(usrstake, get_self());
    _db.set(sysstake, get_self());
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

