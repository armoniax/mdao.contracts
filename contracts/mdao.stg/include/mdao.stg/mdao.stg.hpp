#include <eosio/asset.hpp>
#include <eosio/eosio.hpp>
#include <string>
#include <thirdparty/picomath.hpp>
#include <amax.token/amax.token.hpp>
#include <amax.ntoken/amax.ntoken.hpp>
#include <aplink.token/aplink.token.hpp>

#include <thirdparty/utils.hpp>
#include "mdao.stgdb.hpp"

using std::string;
using namespace eosio;
using namespace wasm::db;
using namespace picomath;

namespace mdao {
class [[eosio::contract("mdaostrateg1")]] strategy : public contract {
private:
    dbc                 _db;
    global_singleton    _global;
    global_t            _gstate;

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
                const string& stg_name, 
                const string& stg_algo,
                const name& type,
                const name& ref_contract,
                const uint64_t& ref_sym);

   /**
    * @brief create a strategy for token/nft balance, 1 weight for account grater than balance_value
    *
    * @param creator - the account to create strategy
    * @param stg_name - name of strategy,
    * @param balance_value - the require token amount of strategy
    * @param type - the type of strategy
    * @param ref_contract - the asset contract account
    * @param ref_sym - the symbol of token/nft, should format as nsymbol or symbol
    */
    [[eosio::action]]
    void balancestg(const name& creator, 
                const string& stg_name, 
                const uint64_t& balance_value,
                const name& type,
                const name& ref_contract,
                const uint64_t& ref_sym);

    [[eosio::action]]
    void setalgo(const name& creator, 
                const uint64_t& stg_id, 
                const string& stg_algo);

    [[eosio::action]]
    void verify(const name& creator,
                   const uint64_t& stg_id, 
                   const uint64_t& value,
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
                 const uint64_t& stg_id);

    [[eosio::action]]
    void formatsym(const symbol_code& sym);

   static int32_t cal_weight(const name& stg_contract_account, 
                  const uint64_t& value, 
                  const name& account, 
                  const uint64_t& stg_id )
   {
        auto db = dbc(stg_contract_account);
        auto stg = strategy_t(stg_id);
        check(db.get(stg), "cannot find strategy");

         PicoMath pm;
         auto &x = pm.addVariable("x");
         x = value;
         auto result = pm.evalExpression(stg.stg_algo.c_str());
         CHECKC(result.isOk(), err::PARAM_ERROR, result.getError());
         int32_t weight = int32_t(floor(result.getResult()));
         return weight;
   }

   static int32_t cal_balance_weight(const name& stg_contract_account,  
                             const uint64_t& stg_id,
                             const name& account )
   {
         auto db = dbc(stg_contract_account);
         auto stg = strategy_t(stg_id);
         check(db.get(stg), "cannot find strategy");

         uint64_t value = 0;
         switch (stg.type.value)
         {
         case strategy_type::tokenbalance.value: {
            symbol_code sym_code(stg.ref_sym);
            value = eosio::token::get_balance(stg.ref_contract, account, sym_code).amount;
            break;
         }
         case strategy_type::nftbalance.value:{
            amax::nsymbol sym(stg.ref_sym);
            value = amax::ntoken::get_balance(stg.ref_contract, account, sym).amount;
            break;
         }
         case strategy_type::nparentbalanc.value:{
            amax::nsymbol sym(stg.ref_sym);
            value = amax::ntoken::get_balance_by_parent(stg.ref_contract, account, (uint32_t)sym.raw());
            break;
         }
         case strategy_type::tokensum.value: {
            symbol_code sym_code(stg.ref_sym);
            value = aplink::token::get_sum(stg.ref_contract, account, sym_code).amount;
            break;
         }
         default:
            check(false, "unsupport calculating type");
            break;
         }

         PicoMath pm;
         auto &x = pm.addVariable("x");
         x = value;
         auto result = pm.evalExpression(stg.stg_algo.c_str());
         CHECKC(result.isOk(), err::PARAM_ERROR, result.getError());
         int32_t weight = int32_t(floor(result.getResult()));

         // check(false, " balance: " + to_string(value) + " | weight: "+ to_string(weight));
         return weight;
   }

   static int32_t cal_stake_weight(const name& stg_contract_account,  
                             const uint64_t& stg_id,
                             const name& stake_contract,
                             const name& account )
   {
         auto db = dbc(stg_contract_account);
         auto stg = strategy_t(stg_id);
         check(db.get(stg), "cannot find strategy");

         uint64_t value = 0;
         switch (stg.type.value)
         {
         case strategy_type::tokenstake.value: {
            break;
         }
         case strategy_type::nftstake.value:{
            break;
         }
         case strategy_type::nparentstake.value:{
            break;
         }
         default:
            check(false, "unsupport calculating type");
            break;
         }

         PicoMath pm;
         auto &x = pm.addVariable("x");
         x = value;
         auto result = pm.evalExpression(stg.stg_algo.c_str());
         CHECKC(result.isOk(), err::PARAM_ERROR, result.getError());
         int32_t weight = int32_t(floor(result.getResult()));

         // check(false, " balance: " + to_string(value) + " | weight: "+ to_string(weight));
         return weight;
   }
};
}