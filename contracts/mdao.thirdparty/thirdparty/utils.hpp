#pragma once

#include <string>
#include <algorithm>
#include <iterator>
#include <eosio/eosio.hpp>
#include "safe.hpp"

#include <eosio/asset.hpp>

using namespace std;

#define EMPTY_MACRO_FUNC(...)

#define PP(prop) "," #prop ":", prop
#define PP0(prop) #prop ":", prop
#define PRINT_PROPERTIES(...) eosio::print("{", __VA_ARGS__, "}")

#ifndef ASSERT
    #define ASSERT(exp) eosio::check(exp, #exp)
#endif

#ifndef TRACE
    #define TRACE(...) print(__VA_ARGS__)
#endif

#define TRACE_L(...) TRACE(__VA_ARGS__, "\n")

#define CHECK(exp, msg) { if (!(exp)) eosio::check(false, msg); }
#define CHECKC(exp, code, msg) \
   { if (!(exp)) eosio::check(false, string("$$$") + to_string((int)code) + string("$$$ ") + msg); }

enum class err: uint8_t {
   NONE                 = 0,
   RECORD_NOT_FOUND     = 1,
   RECORD_EXISTING      = 2,
   ASSET_MISMATCH       = 3,
   SYMBOL_MISMATCH      = 4,
   PARAM_ERROR          = 5,
   PAUSED               = 6,
   NO_AUTH              = 7,
   NOT_POSITIVE         = 8,
   NOT_STARTED          = 9,
   OVERSIZED            = 10,
   TIME_EXPIRED         = 11,
   NOTIFY_UNRELATED     = 12,
   ACTION_REDUNDANT     = 13,
   ACCOUNT_INVALID      = 14,
   UN_INITIALIZE        = 16,
   HAS_INITIALIZE       = 17,
   UNRESPECT_RESULT     = 18,
   MAINTAINING          = 19,
   STATUS_MISMATCH      = 20,
   AMOUNT_TOO_SMALL     = 21
};


template<typename T>
int128_t multiply(int128_t a, int128_t b) {
    int128_t ret = a * b;
    CHECK(ret >= std::numeric_limits<T>::min() && ret <= std::numeric_limits<T>::max(),
          "overflow exception of multiply");
    return ret;
}

template<typename T>
int128_t divide_decimal(int128_t a, int128_t b, int128_t precision) {
    int128_t tmp = 10 * a * precision  / b;
    CHECK(tmp >= std::numeric_limits<T>::min() && tmp <= std::numeric_limits<T>::max(),
          "overflow exception of divide_decimal");
    return (tmp + 5) / 10;
}

template<typename T>
int128_t multiply_decimal(int128_t a, int128_t b, int128_t precision) {
    int128_t tmp = 10 * a * b / precision;
    CHECK(tmp >= std::numeric_limits<T>::min() && tmp <= std::numeric_limits<T>::max(),
          "overflow exception of multiply_decimal");
    return (tmp + 5) / 10;
}

#define divide_decimal64(a, b, precision) divide_decimal<int64_t>(a, b, precision)
#define multiply_decimal64(a, b, precision) multiply_decimal<int64_t>(a, b, precision)
#define multiply_i64(a, b) multiply<int64_t>(a, b)


inline constexpr int64_t power(int64_t base, int64_t exp) {
    int64_t ret = 1;
    while( exp > 0  ) {
        ret *= base; --exp;
    }
    return ret;
}

inline constexpr int64_t power10(int64_t exp) {
    return power(10, exp);
}

inline constexpr int64_t calc_precision(int64_t digit) {
    return power10(digit);
}

inline int64_t get_precision(const symbol &s) {
    int64_t digit = s.precision();
    CHECK(digit >= 0 && digit <= 18, "precision digit " + std::to_string(digit) + " should be in range[0,18]");
    return calc_precision(digit);
}

string_view trim(string_view sv) {
    sv.remove_prefix(std::min(sv.find_first_not_of(" "), sv.size())); // left trim
    sv.remove_suffix(std::min(sv.size()-sv.find_last_not_of(" ")-1, sv.size())); // right trim
    return sv;
}

vector<string_view> split(string_view str, string_view delims = " ")
{
    vector<string_view> res;
    std::size_t current, previous = 0;
    current = str.find_first_of(delims);
    while (current != std::string::npos) {
        res.push_back(trim(str.substr(previous, current - previous)));
        previous = current + 1;
        current = str.find_first_of(delims, previous);
    }
    res.push_back(trim(str.substr(previous, current - previous)));
    return res;
}

bool starts_with(string_view sv, string_view s) {
    return sv.size() >= s.size() && sv.compare(0, s.size(), s) == 0;
}

template <class T>
void to_int(string_view sv, T& res) {
    res = 0;
    T p = 1;
    for( auto itr = sv.rbegin(); itr != sv.rend(); ++itr ) {
        CHECK( *itr <= '9' && *itr >= '0', "invalid numeric character of int");
        res += p * T(*itr-'0');
        p   *= T(10);
    }
}

int64_t to_int64(string_view s, const char* err_title) {
    errno = 0;
    uint64_t ret = std::strtoll(s.data(), nullptr, 10);
    CHECK(errno == 0, string(err_title) + ": convert str to int64 error: " + std::strerror(errno));
    return ret;
}

uint64_t to_uint64(string_view s, const char* err_title) {
    errno = 0;
    uint64_t ret = std::strtoul(s.data(), nullptr, 10);
    CHECK(errno == 0, string(err_title) + ": convert str to uint64 error: " + std::strerror(errno));
    return ret;
}

template<typename CharT>
static std::string to_hex(const CharT* d, uint32_t s) {
  std::string r;
  const char* to_hex="0123456789abcdef";
  uint8_t* c = (uint8_t*)d;
  for( uint32_t i = 0; i < s; ++i ) {
    (r += to_hex[(c[i] >> 4)]) += to_hex[(c[i] & 0x0f)];
  }
  return r;
}

template <class T>
void precision_from_decimals(int8_t decimals, T& p10)
{
    CHECK(decimals <= 18, "precision should be <= 18");
    p10 = 1;
    T p = decimals;
    while( p > 0  ) {
        p10 *= 10; --p;
    }
}

asset asset_from_string(string_view from)
{
    string_view s = trim(from);

    // Find space in order to split amount and symbol
    auto space_pos = s.find(' ');
    CHECK(space_pos != string::npos, "Asset's amount and symbol should be separated with space");
    auto symbol_str = trim(s.substr(space_pos + 1));
    auto amount_str = s.substr(0, space_pos);

    // Ensure that if decimal point is used (.), decimal fraction is specified
    auto dot_pos = amount_str.find('.');
    if (dot_pos != string::npos) {
        CHECK(dot_pos != amount_str.size() - 1, "Missing decimal fraction after decimal point");
    }

    // Parse symbol
    uint8_t precision_digit = 0;
    if (dot_pos != string::npos) {
        precision_digit = amount_str.size() - dot_pos - 1;
    }

    symbol sym = symbol(symbol_str, precision_digit);

    // Parse amount
    safe<int64_t> int_part, fract_part;
    if (dot_pos != string::npos) {
        to_int(amount_str.substr(0, dot_pos), int_part);
        to_int(amount_str.substr(dot_pos + 1), fract_part);
        if (amount_str[0] == '-') fract_part *= -1;
    } else {
        to_int(amount_str, int_part);
    }

    safe<int64_t> amount = int_part;
    safe<int64_t> precision; precision_from_decimals(sym.precision(), precision);
    amount *= precision;
    amount += fract_part;

    return asset(amount.value, sym);
}

template<typename charT>
struct my_equal {
    bool operator()(charT ch1, charT ch2) {
        return std::toupper(ch1) == std::toupper(ch2);
    }
};

template<typename T>
int find_substr( const T& str1, const T& str2 )
{
    typename T::const_iterator it = std::search( str1.begin(), str1.end(), 
        str2.begin(), str2.end(), my_equal<typename T::value_type>() );
    if ( it != str1.end() ) return it - str1.begin();
    else return -1; // not found
}

uint8_t from_hex( char c ) {
    if( c >= '0' && c <= '9' )
    return c - '0';
    if( c >= 'a' && c <= 'f' )
        return c - 'a' + 10;
    if( c >= 'A' && c <= 'F' )
        return c - 'A' + 10;
    check( false, "Invalid hex character");
    return 0;
}

size_t from_hex( const string& hex_str, char* out_data, size_t out_data_len ) {
    string::const_iterator i = hex_str.begin();
    uint8_t* out_pos = (uint8_t*)out_data;
    uint8_t* out_end = out_pos + out_data_len;
    while( i != hex_str.end() && out_end != out_pos ) {
        *out_pos = from_hex( *i ) << 4;   
        ++i;
        if( i != hex_str.end() )  {
            *out_pos |= from_hex( *i );
            ++i;
        }
        ++out_pos;
    }
    return out_pos - (uint8_t*)out_data;
}

