#pragma once

#include <eosio/asset.hpp>
#include <eosio/singleton.hpp>
#include <eosio/privileged.hpp>
#include <eosio/name.hpp>
#include <eosio/action.hpp>
#include <map>

namespace mdao {

using namespace std;
using namespace eosio;

#define TREASURY_TG_TBL [[eosio::table, eosio::contract("mdaotreasury")]]


// struct nsymbol {
//     uint32_t id;
//     uint32_t parent_id;

//     nsymbol() {}
//     nsymbol(const uint32_t& i): id(i),parent_id(0) {}
//     nsymbol(const uint32_t& i, const uint32_t& pid): id(i),parent_id(pid) {}

//     friend bool operator==(const nsymbol&, const nsymbol&);
//     friend bool operator<(const nsymbol&, const nsymbol&);
//     bool is_valid()const { return( id > parent_id ); }
//     uint64_t raw()const { return( (uint64_t) parent_id << 32 | id ); } 

//     EOSLIB_SERIALIZE( nsymbol, (id)(parent_id) )
// };

// bool operator==(const nsymbol& symb1, const nsymbol& symb2) { 
//     return( symb1.id == symb2.id && symb1.parent_id == symb2.parent_id ); 
// }
// bool operator<(const nsymbol& symb1, const nsymbol& symb2) { 
//     return( symb1.id < symb2.id && symb1.parent_id < symb2.parent_id ); 
// }


struct TREASURY_TG_TBL treasury_balance_t {
    name dao_code;
    map<extended_symbol, uint64_t> stake_assets;
    map<amax::nsymbol, uint64_t> stake_nfts;
    uint64_t    primary_key()const { return dao_code.value; }
    uint64_t    scope() const { return 0; }

    treasury_balance_t() {}
    treasury_balance_t(const name& code): dao_code(code) {}

    EOSLIB_SERIALIZE( treasury_balance_t, (dao_code)(stake_assets)(stake_nfts))

    typedef eosio::multi_index< "balance"_n, treasury_balance_t > idx_t;

};


} //amax