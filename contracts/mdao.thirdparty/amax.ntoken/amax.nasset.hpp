#pragma once

#include <eosio/asset.hpp>
// #include <deque>
#include "amax.nsymbol.hpp"

namespace amax {

using namespace std;
using namespace eosio;
struct nasset {
    int64_t         amount;
    nsymbol         symbol;

    nasset() {}
    nasset(const uint32_t& id): symbol(id), amount(0) {}
    nasset(const uint32_t& id, const uint32_t& pid): symbol(id, pid), amount(0) {}
    nasset(const uint32_t& id, const uint32_t& pid, const int64_t& am): symbol(id, pid), amount(am) {}
    nasset(const int64_t &amt, const nsymbol &symb) : amount(amt), symbol(symb) {}

    // Unary minus operator
    nasset operator-()const {
        nasset r = *this;
        r.amount = -r.amount;
         return r;
    }

    // Subtraction operator
    friend nasset operator - ( const nasset& a, const nasset& b ) {
        nasset result = a;
        result -= b;
        return result;
    }

    // Addition operator
    friend nasset operator + ( const nasset& a, const nasset& b ) {
        nasset result = a;
        result += b;
        return result;
    }

    nasset& operator+=(const nasset& quantity) { 
        check( quantity.symbol.raw() == this->symbol.raw(), "nsymbol mismatch");
        this->amount += quantity.amount; return *this;
    } 
    nasset& operator-=(const nasset& quantity) { 
        check( quantity.symbol.raw() == this->symbol.raw(), "nsymbol mismatch");
        this->amount -= quantity.amount; return *this; 
    }

    
    /**
     * Less than operator
     *
     * @param a - The first asset to be compared
     * @param b - The second asset to be compared
     * @return true - if the first asset's amount is less than the second asset amount
     * @return false - otherwise
     * @pre Both asset must have the same symbol
     */
    friend bool operator<( const nasset& a, const nasset& b ) {
        eosio::check( a.symbol == b.symbol, "comparison of assets with different symbols is not allowed" );
        return a.amount < b.amount;
    }

    /// Comparison operator
    friend bool operator==( const nasset& a, const nasset& b ) {
        eosio::check( a.symbol == b.symbol, "comparison of assets with different symbols is not allowed" );
        return a.amount == b.amount;
    }

    /// Comparison operator
    friend bool operator!=( const nasset& a, const nasset& b ) {
        eosio::check( a.symbol == b.symbol, "comparison of assets with different symbols is not allowed" );
        return a.amount != b.amount;

    }

    /**
     * Less or equal to operator
     *
     * @param a - The first asset to be compared
     * @param b - The second asset to be compared
     * @return true - if the first asset's amount is less or equal to the second asset amount
     * @return false - otherwise
     * @pre Both asset must have the same symbol
     */
    friend bool operator<=( const nasset& a, const nasset& b ) {
        eosio::check( a.symbol == b.symbol, "comparison of assets with different symbols is not allowed" );
        return a.amount <= b.amount;
    }

    /**
     * Greater than operator
     *
     * @param a - The first asset to be compared
     * @param b - The second asset to be compared
     * @return true - if the first asset's amount is greater than the second asset amount
     * @return false - otherwise
     * @pre Both asset must have the same symbol
     */
    friend bool operator>( const nasset& a, const nasset& b ) {
        eosio::check( a.symbol == b.symbol, "comparison of assets with different symbols is not allowed" );
        return a.amount > b.amount;
    }

    /**
     * Greater or equal to operator
     *
     * @param a - The first asset to be compared
     * @param b - The second asset to be compared
     * @return true - if the first asset's amount is greater or equal to the second asset amount
     * @return false - otherwise
     * @pre Both asset must have the same symbol
     */
    friend bool operator>=( const nasset& a, const nasset& b ) {
        eosio::check( a.symbol == b.symbol, "comparison of assets with different symbols is not allowed" );
        return a.amount >= b.amount;
    }


    bool is_valid()const { return symbol.is_valid(); }
    
    EOSLIB_SERIALIZE( nasset, (amount)(symbol) )
};


/**
 *  Extended asset which stores the information of the owner of the asset
 *
 *  @ingroup asset
 */
struct extended_nasset {
    /**
     * The asset
     */
    nasset quantity;

    /**
     * The owner of the asset
     */
    name contract;

    /**
     * Get the extended symbol of the asset
     *
     * @return extended_symbol - The extended symbol of the asset
     */
    extended_nsymbol get_extended_nsymbol()const { return extended_nsymbol{ quantity.symbol, contract }; }

    /**
     * Default constructor
     */
    extended_nasset() = default;

    /**
     * Construct a new extended asset given the amount and extended symbol
     */
    extended_nasset( int64_t v, extended_nsymbol s ):quantity(v,s.get_nsymbol()),contract(s.get_contract()){}
    /**
     * Construct a new extended asset given the asset and owner name
     */
    extended_nasset( nasset a, name c ):quantity(a),contract(c){}

    /// @cond OPERATORS

    // Unary minus operator
    extended_nasset operator-()const {
        return {-quantity, contract};
    }

    // Subtraction operator
    friend extended_nasset operator - ( const extended_nasset& a, const extended_nasset& b ) {
        eosio::check( a.contract == b.contract, "type mismatch" );
        return {a.quantity - b.quantity, a.contract};
    }

    // Addition operator
    friend extended_nasset operator + ( const extended_nasset& a, const extended_nasset& b ) {
        eosio::check( a.contract == b.contract, "type mismatch" );
        return {a.quantity + b.quantity, a.contract};
    }

    /// Addition operator.
    friend extended_nasset& operator+=( extended_nasset& a, const extended_nasset& b ) {
        eosio::check( a.contract == b.contract, "type mismatch" );
        a.quantity += b.quantity;
        return a;
    }

    /// Subtraction operator.
    friend extended_nasset& operator-=( extended_nasset& a, const extended_nasset& b ) {
        eosio::check( a.contract == b.contract, "type mismatch" );
        a.quantity -= b.quantity;
        return a;
    }

    /// Less than operator
    friend bool operator<( const extended_nasset& a, const extended_nasset& b ) {
        eosio::check( a.contract == b.contract, "type mismatch" );
        return a.quantity < b.quantity;
    }

    /// Comparison operator
    friend bool operator==( const extended_nasset& a, const extended_nasset& b ) {
        return std::tie(a.quantity, a.contract) == std::tie(b.quantity, b.contract);
    }

    /// Comparison operator
    friend bool operator!=( const extended_nasset& a, const extended_nasset& b ) {
        return std::tie(a.quantity, a.contract) != std::tie(b.quantity, b.contract);
    }

    /// Comparison operator
    friend bool operator<=( const extended_nasset& a, const extended_nasset& b ) {
        eosio::check( a.contract == b.contract, "type mismatch" );
        return a.quantity <= b.quantity;
    }

    /// Comparison operator
    friend bool operator>=( const extended_nasset& a, const extended_nasset& b ) {
        eosio::check( a.contract == b.contract, "type mismatch" );
        return a.quantity >= b.quantity;
    }

    /// @endcond

    EOSLIB_SERIALIZE( extended_nasset, (quantity)(contract) )
};

}