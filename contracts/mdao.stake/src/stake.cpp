#include <mdao.stake/mdao.stake.hpp>
#include <set>

using namespace eosio;

ACTION mdaostake::stakeToken(const name &account, const name &daocode, const map<name, uint64_t> &tokens)
{
    require_auth( account );
    // TODO
    CHECKC( token exist ?, info_err::NOT_AVAILABLE, "token not exist" );
    // TODO
    CHECKC( has token balance ?, info_err::NOT_AVAILABLE, "balance too low" );
    // TODO
    for token in tokens {
        stake
    }
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

ACTION mdaostake::withdrawToken(const name &account, const name &daocode, const map<name, uint64_t> &tokens){
    require_auth( account );
    // TODO
    CHECKC( freeze deadline expire ?, stake_err::UNDEFINED, "still in lock" );
    // TODO
    CHECKC( has lock balance ?, stake_err::UNDEFINED, "no stake balance" );
    // TODO
    for token in tokens {
        withdraw
    }
}

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

ACTION mdaostake::expendUnlockLine(const name &account, const name &daocode, const uint64_t &delay){
    require_auth( account );
    // TODO
    auto new_unlockline = time_point_sec(current_time_point()) + delay;
    time_point_sec freeze_until = time_point_sec(0);
    freeze_until = freeze_until > new_unlockline? freeze_until:new_unlockline;
}
