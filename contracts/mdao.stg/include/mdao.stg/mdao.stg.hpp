#include <eosio/asset.hpp>
#include <eosio/eosio.hpp>
#include <string>
#include <thirdparty/picomath.hpp>
#include <amax.token/amax.token.hpp>
#include <amax.ntoken/amax.ntoken.hpp>
#include <aplink.token/aplink.token.hpp>
#include <mdao.stake/mdao.stake.hpp>

#include <thirdparty/utils.hpp>
#include <amax.system/amax.system.hpp>
#include "mdao.stgdb.hpp"

using std::string;
using namespace eosio;
using namespace wasm::db;
using namespace picomath;
using namespace eosiosystem;


enum class stg_err: uint8_t {
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

namespace mdao {
class [[eosio::contract("mdao.stg")]] strategy : public contract {
private:
   dbc                    _db;
   stg_singleton          _global;
   stg_global_t           _gstate;

   void _check_contract_and_sym( const name& contract,
                                    const refsymbol& ref_symbol,
                                    const name& type);
public:
    using contract::contract;
    strategy(eosio::name receiver, eosio::name code, datastream<const char*> ds):
      _db(_self), contract(receiver, code, ds), _global(_self, _self.value) {
      if (_global.exists()) {
         _gstate = _global.get();
      } else {
         _gstate = stg_global_t{};
      }
    }

    [[eosio::action]]
    void create(const name& creator,
                const uint64_t& stg_id,
                const string& stg_name,
                const string& stg_algo,
                const name& type,
                const name& ref_contract,
                const refsymbol& ref_sym);

   /**
    * @brief create a strategy for token/nft thresholdstg, 1 weight for balance threshold_value
    *
    * @param creator - the account to create strategy
    * @param stg_name - name of strategy,
    * @param threshold_value - the require token amount of strategy
    * @param type - the type of strategy
    * @param ref_contract - the asset contract account
    * @param ref_sym - the symbol of token/nft, should format as nsymbol or symbol
    */
    [[eosio::action]]
    void thresholdstg(const name& creator,
                const string& stg_name,
                const uint64_t& threshold_value,
                const name& type,
                const name& ref_contract,
                const refsymbol& ref_sym);

      /**
    * @brief create a strategy for token/nft balance, 1 weight for 1 weight_value
    *
    * @param creator - the account to create strategy
    * @param stg_name - name of strategy,
    * @param weight_value - the require token amount of strategy
    * @param type - the type of strategy
    * @param ref_contract - the asset contract account
    * @param ref_sym - the symbol of token/nft, should format as nsymbol or symbol
    */
    [[eosio::action]]
    void balancestg(const name& creator,
                const string& stg_name,
                const uint64_t& weight_value,
                const name& type,
                const name& ref_contract,
                const refsymbol& ref_sym);

    [[eosio::action]]
    void verify(const name& creator,
                   const uint64_t& stg_id,
                   const uint64_t& value,
                   const uint64_t& expect_weight);

    [[eosio::action]]
    void publish(const name& creator,
                const uint64_t& stg_id);

    [[eosio::action]]
    void remove(const name& creator,
                const uint64_t& stg_id);


    [[eosio::action]]
    void testalgo(const name& account,
                 const uint64_t& stg_id);

   static int128_t cal_weight(const name& stg_contract_account,
                  const uint64_t& value,
                  const uint64_t& stg_id )
   {
         auto db = dbc(stg_contract_account);
         auto stg = strategy_t(stg_id);
         check(db.get(stg), "cannot find strategy");

         int128_t weight = cal_algo(stg.stg_algo, value);

         return weight;
   }

   static weight_struct cal_balance_weight( const strategy_t& stg,
                                            const name& account,
                                            const int128_t& voting_rate )
   {
         weight_struct weight_st;
         uint64_t value;
         switch (stg.type.value)
         {
         case strategy_type::TOKEN_BALANCE.value: {
            symbol sym = std::get<symbol>(stg.ref_sym);
            value = eosio::token::get_balance(stg.ref_contract, account, sym.code()).amount;
            weight_st.quantity = asset(value, sym);
            if ( voting_rate > 0 ) {
                //min(x-1000,0)
                weight_st.weight  = cal_algo(stg.stg_algo, double(value) * double(voting_rate) / double(power(10, sym.precision()))) ;
            } else {
                //x*1000
                weight_st.weight  = cal_algo(stg.stg_algo, double(value) / double(power(10, sym.precision())));
            }  
            break;
         }
         case strategy_type::NFT_BALANCE.value:{
            nsymbol sym = std::get<nsymbol>(stg.ref_sym);
            value = amax::ntoken::get_balance(stg.ref_contract, account, sym).amount;
            weight_st.quantity = nasset(value, sym);
            weight_st.weight  = cal_algo(stg.stg_algo, value);
            break;
         }
         case strategy_type::NFT_PARENT_BALANCE.value:{
            nsymbol sym = std::get<nsymbol>(stg.ref_sym);
            value = amax::ntoken::get_balance_by_parent(stg.ref_contract, account, sym.id);
            weight_st.weight  = cal_algo(stg.stg_algo, value);
            break;
         }
         case strategy_type::TOKEN_SUM.value: {
            symbol sym = std::get<symbol>(stg.ref_sym);
            value = aplink::token::get_sum(stg.ref_contract, account, sym.code()).amount;
            if ( voting_rate > 0 ) {
                //min(x-1000,0)
                weight_st.weight  = cal_algo(stg.stg_algo, double(value) * double(voting_rate) / double(power(10, sym.precision()))) ;
            } else {
                //x*1000
                weight_st.weight  = cal_algo(stg.stg_algo, double(value) / double(power(10, sym.precision())));
            }            
            break;
         }
         default:
            check(false, "unsupport calculating type");
            break;
         }

         return weight_st;
   }

   static weight_struct cal_stake_weight(const strategy_t& stg,
                             const name& dao_code,
                             const name& stake_contract,
                             const name& account )
   {
         weight_struct weight_st;
         uint64_t value = 0;
         switch (stg.type.value)
         {
         case strategy_type::TOKEN_STAKE.value: {
            symbol sym = std::get<symbol>(stg.ref_sym);
            if (sym == symbol("AMAX",8)) {
                voters_table voter_tbl(AMAX_SYSTEM, AMAX_SYSTEM.value);
                auto voter_itr = voter_tbl.find(account.value);
                if (voter_itr != voter_tbl.end()) {
                    value = voter_itr->votes.amount * 1'0000;
                }
                weight_st.quantity = asset(value, sym); 
            } else {
                map<extended_symbol, int64_t> tokens = mdaostake::get_user_staked_tokens(stake_contract, account, dao_code);
                asset supply = amax::token::get_supply(stg.ref_contract, sym.code());
                value = tokens.at(extended_symbol(supply.symbol, stg.ref_contract));
            }
            break;
         }
         case strategy_type::NFT_STAKE.value:{
            map<extended_nsymbol, int64_t> nfts = mdaostake::get_user_staked_nfts(stake_contract, account, dao_code);
            value = nfts.at(extended_nsymbol(std::get<nsymbol>(stg.ref_sym), stg.ref_contract));
            break;
         }
         case strategy_type::NFT_PARENT_STAKE.value:{
            set<extended_nsymbol> syms = amax::ntoken::get_syms_by_parent(stg.ref_contract,  std::get<nsymbol>(stg.ref_sym).parent_id );
            map<extended_nsymbol, int64_t> nfts = mdaostake::get_user_staked_nfts(stake_contract, account, dao_code);
            for (auto itr = syms.begin() ; itr != syms.end(); itr++) {
               if (nfts.count(*itr)) value += nfts.at(*itr);
            }
            break;
         }
         default:
            check(false, "unsupport calculating type");
            break;
         }
         weight_st.weight = cal_algo(stg.stg_algo, value);

         // check(false, " balance: " + to_string(value) + " | weight: "+ to_string(weight));
         return weight_st;
   }

   static int128_t cal_algo(const string& stg_algo,
                            const double& value)
   {
         PicoMath pm;
         auto &x = pm.addVariable("x");
         x = value;
         auto result = pm.evalExpression(stg_algo.c_str());
         CHECKC(result.isOk(), err::PARAM_ERROR, result.getError());
         int128_t weight = int128_t(floor(result.getResult()));

         return weight;
   }
};


}