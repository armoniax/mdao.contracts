#pragma once

#include <eosio/asset.hpp>

namespace amax {

using namespace std;
using namespace eosio;

// struct nsymbol
// {
//     uint32_t id;
//     uint32_t parent_id;

//     nsymbol() {}
//     constexpr nsymbol(const uint32_t &i) : id(i), parent_id(0) {}
//     constexpr nsymbol(const uint32_t &i, const uint32_t &pid) : id(i), parent_id(pid) {}

//     friend bool operator==(const nsymbol &, const nsymbol &);
//     friend bool operator<(const nsymbol &, const nsymbol &);

//     bool is_valid() const { return (id > parent_id); }
//     uint64_t raw() const { return ((uint64_t)parent_id << 32 | id); }

//     EOSLIB_SERIALIZE(nsymbol, (id)(parent_id))
// };

// bool operator==(const nsymbol &symb1, const nsymbol &symb2)
// {
//     return (symb1.id == symb2.id && symb1.parent_id == symb2.parent_id);
// }

// bool operator<(const nsymbol &symb1, const nsymbol &symb2)
// {
//     return (symb1.id < symb2.id || symb1.parent_id < symb2.parent_id);
// }
struct nsymbol {
    uint32_t id;
    uint32_t parent_id;

    nsymbol() {}
    nsymbol(const uint32_t& i): id(i),parent_id(0) {}
    nsymbol(const uint32_t& i, const uint32_t& pid): id(i),parent_id(pid) {}
    nsymbol(const uint64_t& raw): parent_id(raw >> 32), id(raw) {}

    friend bool operator==(const nsymbol&, const nsymbol&);
    bool is_valid()const { return( id > parent_id ); }
    uint64_t raw()const { return( (uint64_t) parent_id << 32 | id ); } 
    EOSLIB_SERIALIZE( nsymbol, (id)(parent_id) )
};

bool operator==(const nsymbol& symb1, const nsymbol& symb2) { 

    return( symb1.id == symb2.id && symb1.parent_id == symb2.parent_id ); 
}
bool operator<(const nsymbol& symb1, const nsymbol& symb2) { 
   
    return( symb1.id < symb2.id ); 
}
/**
 *  Extended nasset which stores the information of the owner of the symbol
 *
 *  @ingroup symbol
 */
class extended_nsymbol
{
public:
    /**
     * Default constructor, construct a new extended_symbol
     */
    extended_nsymbol() {}

    /**
     * Construct a new symbol_code object initialising symbol and contract with the passed in symbol and name
     *
     * @param sym - The symbol
     * @param con - The name of the contract
     */
    extended_nsymbol(nsymbol s, name con) : sym(s), contract(con) {}

    /**
     * Returns the symbol in the extended_contract
     *
     * @return nsymbol
     */
    nsymbol get_nsymbol() const { return sym; }

    /**
     * Returns the name of the contract in the extended_symbol
     *
     * @return name
     */
    name get_contract() const { return contract; }

    /**
     * Equivalency operator. Returns true if a == b (are the same)
     *
     * @return boolean - true if both provided extended_symbols are the same
     */
    friend bool operator == ( const extended_nsymbol& a, const extended_nsymbol& b ) {
        return std::tie( a.sym, a.contract ) == std::tie( b.sym, b.contract );
    }

    /**
     * Inverted equivalency operator. Returns true if a != b (are different)
     *
     * @return boolean - true if both provided extended_symbols are not the same
     */
    friend bool operator != ( const extended_nsymbol& a, const extended_nsymbol& b ) {
        return std::tie( a.sym, a.contract ) != std::tie( b.sym, b.contract );
    }

    /**
     * Less than operator. Returns true if a < b.
     *
     * @return boolean - true if extended_symbol `a` is less than `b`
     */
    friend bool operator < ( const extended_nsymbol& a, const extended_nsymbol& b ) {
        return std::tie( a.sym, a.contract ) < std::tie( b.sym, b.contract );
    }

private:
    nsymbol sym;   ///< the symbol
    name contract; ///< the token contract hosting the symbol

    EOSLIB_SERIALIZE(extended_nsymbol, (sym)(contract))
};

}