#pragma once

#include <eosio/asset.hpp>
#include <eosio/eosio.hpp>
#include <mdao.conf/mdao.conf.hpp>
#include <string>

namespace eosiosystem {
   class system_contract;
}
static constexpr name AMAX_CUSTODY{"amax.custody"_n};

enum class factory_err: uint8_t {
    DID_NOT_AUTH        = 1,
    CODE_REPEAT         = 2,
    NOT_AVAILABLE       = 3,
    NOT_ALLOW           = 4,
    TOKEN_NOT_EXIST     = 5,
    AMAX_NOT_ENOUGH     = 6,
    STATE_MISMATCH      = 7
};


namespace mdaotokenfactory
{
    using std::string;
    using std::map;
    using namespace eosio;


    struct memo_params {
        string_view fullname;
        asset       maximum_supply;
        string_view metadata;
        EOSLIB_SERIALIZE(memo_params, (fullname)(maximum_supply)(metadata) )
    };

    class [[eosio::contract("tokenfactory")]] tokenfactory : public contract
    {

    using conf_t = mdao::conf_global_t;
    using conf_table_t = mdao::conf_global_singleton;

    using conf_t2 = mdao::conf_global_t2;
    using conf_table_t2 = mdao::conf_global_singleton2;

    public:
        using contract::contract;

    /**
        * @brief user creat token
        *
        * @param from - token creator
        * @param to - fee collector,
        * @param quantity - creat token fee
        * @param memo - format: fullname|asset|metadata|planid
        */
        [[eosio::on_notify("amax.token::transfer")]]
        void ontransfer(const name& from, const name& to, const asset& quantity, const string& memo);

        [[eosio::action]]
        void issuetoken(const name& owner,const name& to,
                            const asset& quantity, const string& memo);

        using issue_action = eosio::action_wrapper<"issue"_n, &tokenfactory::issuetoken>;

    private:
        std::unique_ptr<conf_table_t> _conf_tbl_ptr;
        std::unique_ptr<conf_t> _conf_ptr;

        std::unique_ptr<conf_table_t2> _conf_tbl_ptr2;
        std::unique_ptr<conf_t2> _conf_ptr2;

        const conf_t& _conf();
        const conf_t2& _conf2();

        memo_params _memo_analysis(const string& memo,  const conf_t& conf );
        void _did_auth_check( const name& from );
        void _custody_check( const name& from, const conf_t2& conf);

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
