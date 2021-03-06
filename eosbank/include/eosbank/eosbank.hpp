// #pragma once

#include <eosiolib/asset.hpp>
#include <eosiolib/eosio.hpp>

#include <string>

namespace eosiosystem {
   class system_contract;
}

namespace eosio {

   using std::string;

   class [[eosio::contract("eosbank")]] bank : public contract {
      public:
         using contract::contract;

         bank( name        receiver,
               name        code,
               datastream  <const char*> ds);

         [[eosio::action]]
         void initconfig( name     oracles,
                          name     liquidator,
                          float    eos_price,
                          float    collateral_ratio,
                          float    liquidation_duration);


         [[eosio::action]]
         void setconfig( uint8_t    type,
                         float      value);

         [[eosio::action]]
         void depositeos( name   from,
                          asset  quantity);

         [[eosio::action]]
         void geteos( name   from,
                      name   to,
                      asset  quantity,
                      string memo);

         [[eosio::action]]
         void geteod( name   from,
                      name   to,
                      asset  quantity,
                      string memo);

         [[eosio::action]]
         void depositeod( name   from,
                          asset  amount);

         [[eosio::action]]
         void getloan( name   user,
                      asset  amount,
                      asset collateral);

         [[eosio::action]]
         void incrsclltrl( name user,
                           uint64_t loanid,
                           asset amount);

         [[eosio::action]]
         void settleloan( name user,
                          uint64_t loanid,
                          asset amount);

         [[eosio::action]]
         void liquidate( name       user,
                         uint64_t   loanid);

         [[eosio::action]]
         void liquidated( uint64_t  loanId,
                          asset     amount,
                          name buyer);


      private:
         #define EOD_SYMBOL symbol( "EOD", 4 )
         #define EOS_SYMBOL symbol( "EOS", 4 )
         const char *INVALID_ADDRESS = "INVALID_ADDRESS";
         const char *ONLY_ORACLES = "ONLY_ORACLE";
         const char *INVALID_AMOUNT = "INVALID_AMOUNT";
         const char *COLLATERAL_NOT_ENOUGH = "COLLATERAL_NOT_ENOUGH";
         const char *ONLY_LOAN_OWNER = "ONLY_LOAN_OWNER";
         const char *NOT_ENOUGH_ALLOWANCE = "NOT_ENOUGH_ALLOWANCE";
         const char *NOT_ACTIVE_LOAN = "NOT_ACTIVE_LOAN";
         const char *ENOUGH_COLLATERAL = "ENOUGH_COLLATERAL";
         const char *ONLY_LIQUIDATOR = "ONLY_LIQUIDATOR";
         const char *NOT_CONFIG_YET = "Contract is Not Config Yet";

         struct [[eosio::table]] config_table {
            uint64_t id;
            name     oracle_address;
            name     liquidator_add;
            float    eos_price;
            float    collateral_ratio;
            float    liquidation_duration;

            uint64_t primary_key() const { return id; }
         };

         struct [[eosio::table]] loan_table {
            uint64_t id;
            name     debtor;
            asset    collateral;
            asset    amount;
            uint8_t  state;

            uint64_t primary_key() const { return id; }
         };

         struct [[eosio::table]] trust_fund {
            name     user;
            asset    eos_fund;
            asset    eod_fund;

            uint64_t primary_key() const { return user.value; }
         };

         typedef eosio::multi_index< "config"_n, config_table > config;
         typedef eosio::multi_index< "loan"_n, loan_table > loan;
         typedef eosio::multi_index< "trustfund"_n, trust_fund > trustfund;

         enum LoanState {
            ACTIVE = 0,
            UNDER_LIQUIDATION,
            LIQUIDATED,
            SETTLED
         };

         enum Types {
            EOS_PRICE = 0,
            COLLATERAL_RATIO,
            LIQUIDATION_DURATION
         };

         void init();
         void enough_collateral( name user, asset amount, asset collateral );

   };

} /// namespace eosbank
