#include "mdao.token/mdao.token.hpp"
#include <thirdparty/utils.hpp>

using namespace std;

namespace mdaotoken {

    static constexpr eosio::name active_permission{"active"_n};

    #define NOTIFYFEE(from, to , fee, memo) \
        {	mdaotoken::token::notifypayfee_action act{ _self, { {_self, active_permission} } };\
                act.send( from, to , fee, memo );}

    #define BURNFEE(account, quantity , memo) \
        {	mdaotoken::token::burnfee_action act{ _self, { {_self, active_permission} } };\
                act.send( account, quantity , memo );}

    #define TOKEN_TRANSFER(bank, to, quantity, memo) \
        { action(permission_level{get_self(), "active"_n }, bank, "transfer"_n, std::make_tuple( _self, to, quantity, memo )).send(); }

// #define CHECK(exp, msg) { if (!(exp)) eosio::check(false, msg); }

//     template<typename Int, typename LargerInt>åå
//     LargerInt multiply_decimal(LargerInt a, LargerInt b, LargerInt precision) {
//         LargerInt tmp = a * b / precision;
//         CHECK(tmp >= std::numeric_limits<Int>::min() && tmp <= std::numeric_limits<Int>::max(),
//             "overflow exception of multiply_decimal");
//         return tmp;
//     }

//     #define multiply_decimal64(a, b, precision) multiply_decimal<int64_t, int128_t>(a, b, precision)

    void token::create(const name &issuer,
                        const asset &maximum_supply,
                        const uint16_t &fee_ratio,
                        const std::string &fullname,
                        const name &dao_code,                       
                        const std::string &meta_data)
    {
        auto conf = _conf();
        check(has_auth(conf.managers[manager_type::INFO]) || has_auth(conf.managers[manager_type::ALGOEX]), "insufficient permissions");
        check(is_account(issuer), "issuer account does not exist");
        
        const auto &sym = maximum_supply.symbol;
        auto sym_code_raw = sym.code().raw();
        check(sym.is_valid(), "invalid symbol name");
        check(maximum_supply.is_valid(), "invalid supply");
        check(maximum_supply.amount > 0, "max-supply must be positive");
        check(fullname.size() <= 24, "max fullname size 24");

        stats statstable(get_self(), sym_code_raw);
        auto existing = statstable.find(sym_code_raw);
        check(existing == statstable.end(), "token with symbol already exists");

        statstable.emplace(get_self(), [&](auto &s) {
            s.supply.symbol     = maximum_supply.symbol;
            s.max_supply        = maximum_supply;
            s.issuer            = issuer;
            s.dao_code          = dao_code;
            s.fee_ratio         = fee_ratio;
            s.min_fee_quantity  = asset(0, maximum_supply.symbol);
            s.fullname = fullname;
            s.meta_data = meta_data;
        });
    }

    void token::issue(const name &to, const asset &quantity, const string &memo)
    {

        const auto& sym = quantity.symbol;
        auto sym_code_raw = sym.code().raw();
        check(sym.is_valid(), "invalid symbol name");
        check(memo.size() <= 256, "memo has more than 256 bytes");

        stats statstable(get_self(), sym_code_raw);
        auto existing = statstable.find(sym_code_raw);
        check(existing != statstable.end(), "token with symbol does not exist, create token before issue");
        const auto &st = *existing;
        check( has_auth(to) && (st.issuer == to || MDAO_TREASURY == to || MDAO_ALGOEX == to), "insufficient permissions");

        check(quantity.is_valid(), "invalid quantity");
        check(quantity.amount > 0, "must issue positive quantity");

        check(quantity.symbol == st.supply.symbol, "symbol precision mismatch");
        check(quantity.amount <= st.max_supply.amount - st.supply.amount, "quantity exceeds available supply");

        statstable.modify(st, same_payer, [&](auto &s)
                          { 
                            s.supply += quantity; 
                          });

        add_balance(st, to, quantity, get_self());
        
        accounts accts(get_self(), to.value);
        const auto &acct = accts.get(sym_code_raw, "account of token does not exist");
        accts.modify(acct, get_self(), [&](auto &a) {
             a.is_fee_exempt = true;
        });

        require_recipient(to);
    }

    void token::retire(const asset &quantity, const string &memo)
    {
        const auto& sym = quantity.symbol;
        auto sym_code_raw = sym.code().raw();
        check(sym.is_valid(), "invalid symbol name");
        check(memo.size() <= 256, "memo has more than 256 bytes");

        stats statstable(get_self(), sym_code_raw);
        auto existing = statstable.find(sym_code_raw);
        check(existing != statstable.end(), "token with symbol does not exist");
        const auto &st = *existing;

        require_auth(st.issuer);

        check(quantity.is_valid(), "invalid quantity");
        check(quantity.amount > 0, "must retire positive quantity");

        check(quantity.symbol == st.supply.symbol, "symbol precision mismatch");

        statstable.modify(st, same_payer, [&](auto &s)
                          { s.supply -= quantity; });

        sub_balance(st, st.issuer, quantity);
    }

    void token::burnfee(const name& account, const asset &quantity, const string &memo)
    {
        const auto& sym = quantity.symbol;
        auto sym_code_raw = sym.code().raw();
        check(sym.is_valid(), "invalid symbol name");
        check(memo.size() <= 256, "memo has more than 256 bytes");

        stats statstable(get_self(), sym_code_raw);
        auto existing = statstable.find(sym_code_raw);
        check(existing != statstable.end(), "token with symbol does not exist");
        const auto &st = *existing;

        require_auth(get_self());

        check(quantity.is_valid(), "invalid quantity");
        check(quantity.amount > 0, "must retire positive quantity");

        check(quantity.symbol == st.supply.symbol, "symbol precision mismatch");

        statstable.modify(st, same_payer, [&](auto &s)
                          { s.supply -= quantity; });

        sub_balance(st, account, quantity);
    }

void token::transfer( const name&    from,
                      const name&    to,
                      const asset&   quantity,
                      const string&  memo )
    {
        auto conf = _conf();
        check(from != to, "cannot transfer to self");
        require_auth(from);
        check(is_account(to), "to account does not exist");
        auto sym_code_raw = quantity.symbol.code().raw();
        stats statstable(get_self(), sym_code_raw);
        const auto &st = statstable.get(sym_code_raw, "token of symbol does not exist");
        check(st.supply.symbol == quantity.symbol, "symbol precision mismatch");
        check(!st.is_paused, "token is paused");

        require_recipient( from );
        require_recipient( to );

        check(quantity.is_valid(), "invalid quantity");
        check(quantity.amount > 0, "must transfer positive quantity");
        check(quantity.symbol == st.supply.symbol, "symbol precision mismatch");
        check(memo.size() <= 256, "memo has more than 256 bytes");

        CHECK(quantity > st.min_fee_quantity, "quantity must larger than min fee:" + st.min_fee_quantity.to_string());

        asset actual_recv = quantity;
        asset fee = asset(0, quantity.symbol);
        if ( st.fee_ratio > 0 &&  to != st.issuer )
        {
            accounts to_accts(get_self(), to.value);
            auto to_acct = to_accts.find(sym_code_raw);
 
            if ( (to_acct == to_accts.end() && 
                 (to != conf.managers[manager_type::INFO]
                    || to != conf.managers[manager_type::CONF] 
                    || to != conf.managers[manager_type::STRATEGY] 
                    || to != conf.managers[manager_type::GOV] 
                    || to != conf.managers[manager_type::TREASURY] 
                    || to != conf.managers[manager_type::PROPOSAL])
                 ) 
                || !to_acct->is_fee_exempt) {
                fee.amount = std::max( st.min_fee_quantity.amount,
                                (int64_t)multiply_decimal64(quantity.amount, st.fee_ratio, RATIO_BOOST) );
                CHECK(fee < quantity, "the calculated fee must less than quantity");
                actual_recv -= fee;
            }
        }

        auto payer = has_auth(to) ? to : from;

        sub_balance(st, from, quantity, true);
        transfer_add_balance(st, to, actual_recv, payer, conf);

        if (fee.amount > 0) {
            add_balance(st, MDAO_TREASURY, fee, get_self());
            TOKEN_TRANSFER(_self, MDAO_TREASURY, fee, "")
        }
    }

    /**
     * Notify pay fee.
     * Must be Triggered as inline action by transfer()
     *
     * @param from - the from account of transfer(),
     * @param to - the to account of transfer, fee payer,
     * @param fee - the fee of transfer to be payed,
     * @param memo - the memo of the transfer().
     * Require contract auth
     */
    void token::notifypayfee(const name &from, const name &to,  const asset &fee, const string &memo) {
        require_auth(get_self());
        require_recipient(to);
    }

    void token::sub_balance(const currency_stats &st, const name &owner, const asset &value,
                             bool is_check_frozen)
    {
        accounts from_accts(get_self(), owner.value);
        const auto &from = from_accts.get(value.symbol.code().raw(), "no balance object found");
        if (is_check_frozen) {
            check(!is_account_frozen(st, owner, from), "from account is frozen");
        }
        check(from.balance.amount >= value.amount, "overdrawn balance");

        from_accts.modify(from, same_payer, [&](auto &a) {
            a.balance -= value;
        });
    }

    void token::add_balance(const currency_stats &st, const name &owner, const asset &value,
                             const name &ram_payer, bool is_check_frozen)
    {
        accounts to_accts(get_self(), owner.value);
        auto to = to_accts.find(value.symbol.code().raw());
        if (to == to_accts.end())
        {
            to_accts.emplace(ram_payer, [&](auto &a) {
                a.balance = value;
            });
        }
        else
        {
            if (is_check_frozen) {
                check(!is_account_frozen(st, owner, *to), "to account is frozen");
            }
            to_accts.modify(to, same_payer, [&](auto &a) {
                a.balance += value;
            });
        }
    }

    void token::transfer_add_balance(const currency_stats &st, const name &owner, const asset &value,
                             const name &ram_payer, const conf_t& conf )
    {
        accounts to_accts(get_self(), owner.value);
        auto to = to_accts.find(value.symbol.code().raw());
        if (to == to_accts.end())
        {
            to_accts.emplace(ram_payer, [&](auto &a) {
                a.balance = value;

                if(owner == conf.managers.at(manager_type::INFO) 
                    || owner == conf.managers.at(manager_type::CONF) 
                    || owner == conf.managers.at(manager_type::STRATEGY) 
                    || owner == conf.managers.at(manager_type::GOV)
                    || owner == conf.managers.at(manager_type::TREASURY) 
                    || owner == conf.managers.at(manager_type::PROPOSAL)
                ) 
                    a.is_fee_exempt = true;
            });
        }
        else
        {
            check(!is_account_frozen(st, owner, *to), "to account is frozen");
            
            to_accts.modify(to, same_payer, [&](auto &a) {
                a.balance += value;
            });
        }
    }

    void token::open(const name &owner, const symbol &symbol, const name &ram_payer)
    {
        require_auth(ram_payer);

        check(is_account(owner), "owner account does not exist");

        auto sym_code_raw = symbol.code().raw();
        stats statstable(get_self(), sym_code_raw);
        const auto &st = statstable.get(sym_code_raw, "token of symbol does not exist");
        check(st.supply.symbol == symbol, "symbol precision mismatch");
        check(!st.is_paused, "token is paused");

        open_account(owner, symbol, ram_payer);
    }

    bool token::open_account(const name &owner, const symbol &symbol, const name &ram_payer) {
        accounts accts(get_self(), owner.value);
        auto it = accts.find(symbol.code().raw());
        if (it == accts.end())
        {
            accts.emplace(ram_payer, [&](auto &a)
                          { a.balance = asset{0, symbol}; });
            return true;
        }
        return false;
    }

    void token::close(const name &owner, const symbol &symbol)
    {
        require_auth(owner);

        auto sym_code_raw = symbol.code().raw();
        stats statstable(get_self(), sym_code_raw);
        const auto &st = statstable.get(sym_code_raw, "token of symbol does not exist");
        check(st.supply.symbol == symbol, "symbol precision mismatch");
        check(!st.is_paused, "token is paused");

        accounts accts(get_self(), owner.value);
        auto it = accts.find(sym_code_raw);
        check(it != accts.end(), "Balance row already deleted or never existed. Action won't have any effect.");
        check(!is_account_frozen(st, owner, *it), "account is frozen");
        check(it->balance.amount == 0, "Cannot close because the balance is not zero.");
        accts.erase(it);
    }

    void token::ratio(const symbol &symbol, const uint16_t& fee_ratio) {
        check(fee_ratio >= 0 && fee_ratio < RATIO_BOOST, "fee_ratio out of range");
        update_currency_field(symbol, fee_ratio, &currency_stats::fee_ratio);
    }

    void token::minfee(const symbol &symbol, const asset &min_fee_quantity) {
        check(min_fee_quantity.symbol == symbol, "symbol of min_fee_quantity  mismatch");
        check(min_fee_quantity.amount > 100, "amount of min_fee_quantity should grater than 100");
        update_currency_field(symbol, min_fee_quantity, &currency_stats::min_fee_quantity);
    }

    void token::feeexempt(const symbol &symbol, const name &account, bool is_fee_exempt) {
        auto sym_code_raw = symbol.code().raw();
        stats statstable(get_self(), sym_code_raw);
        const auto &st = statstable.get(sym_code_raw, "token of symbol does not exist");
        check(st.supply.symbol == symbol, "symbol precision mismatch");
        require_auth(st.issuer);

        accounts accts(get_self(), account.value);
        const auto &acct = accts.get(sym_code_raw, "account of token does not exist");

        accts.modify(acct, st.issuer, [&](auto &a) {
             a.is_fee_exempt = is_fee_exempt;
        });
    }

    void token::pause(const symbol &symbol, bool is_paused)
    {
        update_currency_field(symbol, is_paused, &currency_stats::is_paused);
    }

    void token::freezeacct(const symbol &symbol, const name &account, bool is_frozen) {
        auto sym_code_raw = symbol.code().raw();
        stats statstable(get_self(), sym_code_raw);
        const auto &st = statstable.get(sym_code_raw, "token of symbol does not exist");
        check(st.supply.symbol == symbol, "symbol precision mismatch");
        require_auth(st.issuer);

        accounts accts(get_self(), account.value);
        const auto &acct = accts.get(sym_code_raw, "account of token does not exist");

        accts.modify(acct, st.issuer, [&](auto &a) {
             a.is_frozen = is_frozen;
        });
    }

    template <typename Field, typename Value>
    void token::update_currency_field(const symbol &symbol, const Value &v, Field currency_stats::*field,
                                       currency_stats *st_out)
    {
        auto sym_code_raw = symbol.code().raw();
        stats statstable(get_self(), sym_code_raw);
        const auto &st = statstable.get(sym_code_raw, "token of symbol does not exist");
        check(st.supply.symbol == symbol, "symbol precision mismatch");
        require_auth(st.issuer);
        statstable.modify(st, same_payer, [&](auto &s) {
            s.*field = v;
            if (st_out != nullptr) *st_out = s;
        });
    }

    const token::conf_t& token::_conf() {
        if (!_conf_ptr) {
            _conf_tbl_ptr = make_unique<conf_table_t>(MDAO_CONF, MDAO_CONF.value);
            check(_conf_tbl_ptr->exists(), "conf table not existed in contract" );
            _conf_ptr = make_unique<conf_t>(_conf_tbl_ptr->get());
        }
        return *_conf_ptr;
    }
} /// namespace amax_token
