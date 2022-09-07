#include <eosio/asset.hpp>
#include <eosio/eosio.hpp>
#include <string>

#include "picomath.hpp"
#include "thirdparty/utils.hpp"
#include "xdao.stgdb.hpp"
#include "eosio.token/eosio.token.hpp"
#include "aplink.token/aplink.token.hpp"

using std::string;
using namespace eosio;
using namespace wasm::db;
using namespace picomath;

namespace xdao {
class [[eosio::contract("xdao.stg")]] strategy : public contract {
private:
    dbc                 _db;
    global_singleton    _global;
    global_t            _gstate;


   static int32_t _cal_stg_weight(const strategy_t& stg, const name& account){
      auto apl_balance = asset(0, APL_SYMBOL);
      if(stg.require_apl.amount > 0) {
         apl_balance = aplink::token::get_sum_balance(APL_BANK, account, APL_SYMBOL.code());
         CHECKC(apl_balance.amount >= stg.require_apl.amount, err::UNRESPECT_RESULT, "required APL not enough: "+apl_balance.to_string())
      }

      if (stg.type == strategy_type::unlimited && apl_balance.amount >= stg.require_apl.amount) 
         return 1;

      if (stg.type == strategy_type::tokenbalance){
         auto balance = eosio::token::get_balance(stg.ref_contract, account, stg.require_symbol_code);
         CHECKC(balance.amount > 0, err::NOT_POSITIVE, "require asset must be positive")
         PicoMath pm;
         auto &x = pm.addVariable("x");
         x = double(balance.amount)/double(power(10, balance.symbol.precision()));
         auto result = pm.evalExpression(stg.stg_algo.c_str());
         CHECKC(result.isOk(), err::PARAM_ERROR, result.getError());
         int32_t weight = int32_t(floor(result.getResult()));
         return weight;
      }

      return 0;
   };

public:
    using contract::contract;
    strategy(eosio::name receiver, eosio::name code, datastream<const char*> ds):
      _db(_self), contract(receiver, code, ds), _global(_self, _self.value) {
      if (_global.exists()) {
         _gstate = _global.get();
      } else {
         _gstate = global_t{};
      }
    }

    [[eosio::action]]
    void create(const name& creator, 
                const name& type, 
                const string& stg_name, 
                const string& stg_algo,
                const asset& require_apl,
                const symbol_code& require_symbol_code,
                const name& ref_contracts);

    [[eosio::action]]
    void setalgo(const name& creator, 
                const uint64_t& stg_id, 
                const string& stg_algo);

    [[eosio::action]]
    void verify(const name& creator,
                   const uint64_t& stg_id, 
                   const name& account,
                   const uint64_t& respect_weight); 

    [[eosio::action]]
    void publish(const name& creator, 
                const uint64_t& stg_id);

    [[eosio::action]]
    void remove(const name& creator, 
                const uint64_t& stg_id);


    [[eosio::action]]
    void testalgo(const name& account,
                 const string& alog,
                 const double& param);

   static int32_t cal_stg_weight( const name& stg_contract_account, const name& account, const uint64_t& stg_id )
    {
        auto db = dbc(stg_contract_account);
        auto stg = strategy_t(stg_id);
        check(db.get(stg), "cannot find strategy");
        return _cal_stg_weight(stg, account);
    }
};
}