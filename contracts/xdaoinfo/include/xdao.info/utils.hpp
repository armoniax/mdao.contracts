#pragma once

#include <eosio/eosio.hpp>
#include <eosio/print.hpp>


#define EMPTY_MACRO_FUNC(...)

#define PP(prop) "," #prop ":", prop
#define PP0(prop) #prop ":", prop
#define PRINT_PROPERTIES(...) eosio::print("{", __VA_ARGS__, "}")

#define CHECK(exp, msg) { if (!(exp)) eosio::check(false, msg); }

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

#define div_decimal_64(a, b, precision) divide_decimal<int64_t>(a, b, precision)
#define mul_decimal_64(a, b, precision) multiply_decimal<int64_t>(a, b, precision)
#define mul_64(a, b) multiply<int64_t>(a, b)
