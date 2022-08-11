#pragma once

#include <eosio/eosio.hpp>
#include <eosio/print.hpp>


#define EMPTY_MACRO_FUNC(...)

#define PP(prop) "," #prop ":", prop
#define PP0(prop) #prop ":", prop
#define PRINT_PROPERTIES(...) eosio::print("{", __VA_ARGS__, "}")

#define CHECK(exp, msg) { if (!(exp)) eosio::check(false, msg); }
#define CHECKC(exp, code, msg) \
   { if (!(exp)) eosio::check(false, string("$$$") + to_string((int)code) + string("$$$ ") + msg); }

#ifndef ASSERT
    #define ASSERT(exp) CHECK(exp, #exp)
#endif

#ifdef PRINT_TRACE
    #warning "PRINT_TRACE should be used for test!!!"
    #define TRACE(...) print(__VA_ARGS__)
#else
    #define TRACE(...)
#endif

#define TRACE_L(...) TRACE(__VA_ARGS__, "\n")


template<typename T>
int128_t multiply(int128_t a, int128_t b) {
    int128_t ret = a * b;
    CHECK(ret >= std::numeric_limits<T>::min() && ret <= std::numeric_limits<T>::max(),
          "overflow exception of multiply");
    return ret;
}

template<typename T>
int128_t divide_decimal(int128_t a, int128_t b, int128_t precision) {
    // with rounding-off method
    int128_t tmp = 10 * a * precision  / b;
    CHECK(tmp >= std::numeric_limits<T>::min() && tmp <= std::numeric_limits<T>::max(),
          "overflow exception of divide_decimal");
    return (tmp + 5) / 10;
}

template<typename T>
int128_t multiply_decimal(int128_t a, int128_t b, int128_t precision) {
    // with rounding-off method
    int128_t tmp = 10 * a * b / precision;
    CHECK(tmp >= std::numeric_limits<T>::min() && tmp <= std::numeric_limits<T>::max(),
          "overflow exception of multiply_decimal");
    return (tmp + 5) / 10;
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

#define div_decimal_64(a, b, precision) divide_decimal<int64_t>(a, b, precision)
#define mul_decimal_64(a, b, precision) multiply_decimal<int64_t>(a, b, precision)
#define mul_64(a, b) multiply<int64_t>(a, b)
