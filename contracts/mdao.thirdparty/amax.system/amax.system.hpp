#pragma once

#include <eosio/asset.hpp>
#include <eosio/binary_extension.hpp>
#include <eosio/privileged.hpp>
#include <eosio/producer_schedule.hpp>
#include <eosio/singleton.hpp>
#include <eosio/system.hpp>
#include <eosio/time.hpp>

#include <deque>
#include <optional>
#include <string>
#include <type_traits>
#include <cmath>

#ifdef CHANNEL_RAM_AND_NAMEBID_FEES_TO_REX
#undef CHANNEL_RAM_AND_NAMEBID_FEES_TO_REX
#endif
// CHANNEL_RAM_AND_NAMEBID_FEES_TO_REX macro determines whether ramfee and namebid proceeds are
// channeled to REX pool. In order to stop these proceeds from being channeled, the macro must
// be set to 0.
#define CHANNEL_RAM_AND_NAMEBID_FEES_TO_REX 1

#define PP(prop) "," #prop ":", prop
#define PP0(prop) #prop ":", prop
#define PRINT_PROPERTIES(...) eosio::print("{", __VA_ARGS__, "}")

#define CHECK(exp, msg) { if (!(exp)) eosio::check(false, msg); }

#ifndef ASSERT
    #define ASSERT(exp) CHECK(exp, #exp)
#endif

#define LESS(a, b)                     (a) < (b) ? true : false
#define LARGER(a, b)                   (a) > (b) ? true : false
#define LESS_OR(a, b, other_compare)   (a) < (b) ? true : (a) > (b) ? false : ( other_compare )
#define LARGER_OR(a, b, other_compare) (a) > (b) ? true : (a) < (b) ? false : ( other_compare )

template<class... Ts> struct overloaded : Ts... { using Ts::operator()...; };
template<class... Ts> overloaded(Ts...) -> overloaded<Ts...>;

static constexpr name AMAX_SYSTEM  = "amax.system"_n;

namespace eosiosystem {

   static constexpr symbol   vote_symbol                 = symbol("VOTE", 4);

   // Voter info. Voter info stores information about the voter:
   // - `owner` the voter
   // - `proxy` the proxy set by the voter, if any
   // - `producers` the producers approved by this voter if no proxy set
   // - `staked` the amount staked
   struct [[eosio::table, eosio::contract("amax.system")]] voter_info {
      name                owner;     /// the voter
      name                proxy;     /// [deprecated] the proxy set by the voter, if any
      std::vector<name>   producers; /// the producers approved by this voter if no proxy set
      int64_t             staked = 0; /// [deprecated] staked of CPU and NET

      //  Every time a vote is cast we must first "undo" the last vote weight, before casting the
      //  new vote weight.  Vote weight is calculated as:
      //  stated.amount * 2 ^ ( weeks_since_launch/weeks_per_year)
      double              last_vote_weight = 0; /// [deprecated] the vote weight cast the last time the vote was updated

      // Total vote weight delegated to this voter.
      double              proxied_vote_weight= 0; /// [deprecated] the total vote weight delegated to this voter as a proxy
      bool                is_proxy = 0; /// [deprecated] whether the voter is a proxy for others


      uint32_t            flags1                = 0;                       /// resource managed flags
      asset               votes                 = asset(0, vote_symbol);   /// elected votes
      block_timestamp     last_unvoted_time;                               /// vote updated time

      uint64_t primary_key()const { return owner.value; }

      enum class flags1_fields : uint32_t {
         ram_managed = 1,
         net_managed = 2,
         cpu_managed = 4
      };

      // explicit serialization macro is not necessary, used here only to improve compilation time
      EOSLIB_SERIALIZE( voter_info, (owner)(proxy)(producers)(staked)(last_vote_weight)
                                    (proxied_vote_weight)(is_proxy)(flags1)(votes)(last_unvoted_time) )
   };

   typedef eosio::multi_index< "voters"_n, voter_info >  voters_table;

}
