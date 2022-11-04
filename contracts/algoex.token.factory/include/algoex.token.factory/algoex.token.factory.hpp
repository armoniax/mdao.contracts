#pragma once

#include <eosio/asset.hpp>
#include <eosio/eosio.hpp>
#include <mdao.conf/mdao.conf.hpp>
#include <string>

namespace eosiosystem {
   class system_contract;
}

enum class factory_err: uint8_t {
    INVALID_FORMAT      = 1,
    DID_NOT_AUTH        = 2,
    TITLE_REPEAT        = 3,
    CODE_REPEAT         = 4,
    RECORD_NOT_FOUND    = 5,
    PERMISSION_DENIED   = 6,
    NOT_AVAILABLE       = 7,
    PARAM_ERROR         = 8,
    SYMBOL_ERROR        = 9,
    RECORD_EXITS        = 10,
    SIZE_TOO_MUCH       = 11,
    STRATEGY_NOT_FOUND  = 12,
    CANNOT_ZERO         = 13,
    GOVERNANCE_NOT_FOUND= 14,
    SYSTEM_ERROR        = 15,
    NO_AUTH             = 16,
    NOT_POSITIVE        = 17,
    NOT_ALLOW           = 18,
    ACCOUNT_NOT_EXITS   = 19,
    TOKEN_NOT_EXIST     = 20,
    AMAX_NOT_ENOUGH     = 21
};
namespace algoextokenfactory
{
    using std::string;
    using std::map;
    using namespace eosio;

    class [[eosio::contract("tokenfactory")]] tokenfactory : public contract
    {
        
    using conf_t = mdao::conf_global_t;
    using conf_table_t = mdao::conf_global_singleton;

    public:
        using contract::contract;

        [[eosio::on_notify("amax.token::transfer")]] 
        ACTION createtoken(const name& from, const name& to, const asset& quantity, const string& memo);
        
        [[eosio::action]]
        ACTION issuetoken(const name& owner, const name& code, const name& to, 
                            const asset& quantity, const string& memo);
        void _check_permission( const name& dao_code, const name& owner, const conf_t& conf );

        using create_action = eosio::action_wrapper<"create"_n, &tokenfactory::createtoken>;
        using issue_action = eosio::action_wrapper<"issue"_n, &tokenfactory::issuetoken>;

    private:
        std::unique_ptr<conf_table_t> _conf_tbl_ptr;
        std::unique_ptr<conf_t> _conf_ptr;
        const conf_t& _conf();

        struct [[eosio::table]] account
        {
            asset balance;
            bool  is_frozen = false;
            bool  is_fee_exempt = false;

            uint64_t primary_key() const { return balance.symbol.code().raw(); }
        };

 
        struct [[eosio::table]] currency_stats
        {
            asset supply;
            asset max_supply;
            name issuer;
            bool is_paused = false;
            name dao_code;
            uint16_t fee_ratio = 0;         // fee ratio, boost 10000
            asset min_fee_quantity;         // min fee quantity
            std::string fullname;
            std::string meta_data;

            uint64_t primary_key() const { return supply.symbol.code().raw(); }
        };

        typedef eosio::multi_index<"accounts"_n, account> accounts;
        typedef eosio::multi_index<"stat"_n, currency_stats> stats;


    };

}
