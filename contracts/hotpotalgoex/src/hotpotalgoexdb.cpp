#include "hotpotalgoexdb.hpp"
#include <cmath>

using namespace wasm::db;

inline constexpr int64_t power(int64_t base, int64_t exp) {
    int64_t ret = 1;
    while( exp > 0  ) {
        ret *= base; --exp;
    }
    return ret;
}
inline int64_t get_precision(const symbol &s) {
    int64_t digit = s.precision();
    check(digit >= 0 && digit <= 18, "precision digit " + std::to_string(digit) + " should be in range[0,18]");
    return power(10, digit);
}

asset market_t::convert_to_exchange_old( extended_asset& c, asset in ) {
      real_type R(algo_params[algo_parma_type::cwsupply]);
      real_type C(c.quantity.amount+in.amount);
      real_type F((algo_params.at(algo_parma_type::cwvalue)/(RATIO_BOOST*1.0))/RATIO_BOOST);
      real_type T(in.amount);
      real_type ONE(1.0);

      real_type E = -R * (ONE - std::pow( ONE + T / C, F) );
      //print( "E: ", E, "\n");
      int64_t issued = int64_t(E);

      algo_params[algo_parma_type::cwsupply] += issued;
      c.quantity.amount += in.amount;

      return asset( issued, BRIDGE_SYMBOL );
   }

asset market_t::convert_from_exchange_old( extended_asset& c, asset in ) {
    check( in.symbol== BRIDGE_SYMBOL, "unexpected asset symbol input" );

    real_type R(algo_params[algo_parma_type::cwsupply] - in.amount);
    real_type C(c.quantity.amount);
    real_type F(RATIO_BOOST/(algo_params.at(algo_parma_type::cwvalue)/(RATIO_BOOST*1.0)));
    real_type E(in.amount);
    real_type ONE(1.0);


    // potentially more accurate: 
    // The functions std::expm1 and std::log1p are useful for financial calculations, for example, 
    // when calculating small daily interest rates: (1+x)n
    // -1 can be expressed as std::expm1(n * std::log1p(x)). 
    // real_type T = C * std::expm1( F * std::log1p(E/R) );
    
    real_type T = C * (std::pow( ONE + E/R, F) - ONE);
    //print( "T: ", T, "\n");
    int64_t out = int64_t(T);

    algo_params[algo_parma_type::cwsupply] -= in.amount;
    c.quantity.amount -= out;

    return asset( out, c.quantity.symbol );
}

asset market_t::convert_old( asset from, symbol to ) {
    auto sell_symbol  = from.symbol;
    auto ex_symbol    = BRIDGE_SYMBOL;
    auto base_symbol  = base_balance.quantity.symbol;
    auto quote_symbol = quote_balance.quantity.symbol;

    //print( "From: ", from, " TO ", asset( 0,to), "\n" );
    //print( "base: ", base_symbol, "\n" );
    //print( "quote: ", quote_symbol, "\n" );
    //print( "ex: ", supply.symbol, "\n" );

    if( sell_symbol != ex_symbol ) {
        if( sell_symbol == base_symbol ) {
            from = convert_to_exchange( base_balance, from );
        } else if( sell_symbol == quote_symbol ) {
            from = convert_to_exchange( quote_balance, from );
        } else { 
            check( false, "invalid sell" );
        }
    } else {
        if( to == base_symbol ) {
            from = convert_from_exchange( base_balance, from ); 
        } else if( to == quote_symbol ) {
            from = convert_from_exchange( quote_balance, from ); 
        } else {
            check( false, "invalid conversion" );
        }
    }

    if( to != from.symbol )
        return convert( from, to );

    return from;
}

asset market_t::convert_to_exchange(extended_asset& reserve, const asset& payment ){
      double S0 = algo_params.at(algo_parma_type::cwsupply);
      double R0 = reserve.quantity.amount;        //5000
      double dR = payment.amount;                 //50
      double F  = (algo_params.at(algo_parma_type::cwvalue)/(RATIO_BOOST*1.0));    //0.5

      double dS = S0 * ( std::pow(1. + dR / R0, F) - 1. );
      if ( dS < 0 ) dS = 0; // rounding errors
      reserve.quantity += payment;
      algo_params[algo_parma_type::cwsupply]   += int64_t(dS);
      return asset( int64_t(dS), BRIDGE_SYMBOL );
}

asset market_t::convert_from_exchange(extended_asset& reserve, const asset& tokens ){
      const double R0 = reserve.quantity.amount; //999000
      const double S0 = algo_params.at(algo_parma_type::cwsupply);
      const double dS = -tokens.amount; //-49875621
      const double Fi = double(1) / (algo_params.at(algo_parma_type::cwvalue)/(RATIO_BOOST*1.0));

      double dR = R0 * ( std::pow(1. + dS / S0, Fi) - 1. ); // dR < 0 since dS < 0
      if ( dR > 0 ) dR = 0; // rounding errors
      reserve.quantity.amount -= int64_t(-dR);
      algo_params[algo_parma_type::cwsupply] -= tokens.amount;
      return asset( int64_t(-dR), reserve.get_extended_symbol().get_symbol());
}

asset market_t::poly_from_exchange(const asset& in){
    double dY = double(in.amount)/get_precision(in.symbol);
    double X1 = double(base_supply.amount - base_balance.quantity.amount) / get_precision(base_supply.symbol);
    double A = double(algo_params.at(algo_parma_type::aslope)) / SLOPE_BOOST;
    double B = 2 * double(algo_params.at(algo_parma_type::bvalue)) / SLOPE_BOOST;
    double C = - A * power(X1, 2) - B * X1 - 2 * dY;
    
    double X2 = ( - B + sqrt(power(B, 2) - 4 * A * C)) / (2 * A);
    double dX = (X2 - X1) * get_precision(base_supply.symbol);
    if ( dX < 0 || base_balance.quantity.amount < dX) dX = 0;

    asset out = asset( int64_t(dX), base_balance.quantity.symbol );
    base_balance.quantity -= out;
    quote_balance.quantity += in;

    return out;
}

asset market_t::poly_to_exchange(const asset& in){
    double A = double(algo_params.at(algo_parma_type::aslope)) / SLOPE_BOOST;
    double B = double(algo_params.at(algo_parma_type::bvalue)) / SLOPE_BOOST;
    double X1 = double(base_supply.amount - base_balance.quantity.amount) / get_precision(base_supply.symbol);
    double dX = in.amount / get_precision(in.symbol);
    double X2 = X1 - dX;

    double dC = (A * (X1 + X2) + 2 * B) * dX / 2 * get_precision(quote_supply.symbol);
    if ( dC < 0 || dC > quote_balance.quantity.amount) dC = 0;
    asset out( int64_t(dC), quote_balance.quantity.symbol );
    quote_balance.quantity -= out;
    base_balance.quantity += in;

    return out;
}

asset market_t::convert(const asset& from, const symbol& to)
{
    const auto& sell_symbol  = from.symbol;
    const auto& base_symbol  = base_balance.quantity.symbol;
    const auto& quote_symbol = quote_balance.quantity.symbol;

    check( sell_symbol != to, "cannot convert to the same symbol" );
    asset out( 0, to );

    switch (algo_type.value)
    {
    case algo_type_t::polycurve.value: {
        if( sell_symbol == base_symbol && to == quote_symbol ){
            return poly_to_exchange(from);
        } 
        else if ( sell_symbol == quote_symbol && to == base_symbol ) {
            return poly_from_exchange(from);
        }
        else {
            check( false, "invalid conversion" );
        }
        }
        return out;
    case algo_type_t::bancor.value: {
        if ( sell_symbol == base_symbol && to == quote_symbol ) {
            const asset tmp = convert_to_exchange(base_balance, from );
            out = convert_from_exchange(quote_balance, tmp );
            // asset avg = asset((int64_t)divide_decimal64(out.amount, from.amount, power10(out.symbol.precision())), out.symbol);
        } else if ( sell_symbol == quote_symbol && to == base_symbol ) {
            const asset tmp = convert_to_exchange(quote_balance, from );
            out = convert_from_exchange(base_balance, tmp );
        } else {
            check( false, "invalid conversion" );
        }
        return out;
    }
    default:
        return asset(0, to);
    }
}
