// #pragma once

#include <eosiolib/asset.hpp>
#include <eosiolib/eosio.hpp>

#include <string>

namespace eosiosystem {
   class system_contract;
}

namespace eosio {

   using std::string;

   class [[eosio::contract("liquidator")]] liquidator : public contract {
      public:
         using contract::contract;

         liquidator( name        receiver,
               name        code,
               datastream  <const char*> ds);


      private:
         #define MYT_SYMBOL symbol( "MYT", 4 )
         #define EOS_SYMBOL symbol( "EOS", 4 )
         const char *INVALID_ADDRESS =          "INVALID_ADDRESS";
         const char *ONLY_ETHER_BANK =          "ONLY_ETHER_BANK";
         const char *NO_DEPOSIT =               "NO_DEPOSIT";
         const char *INVALID_AMOUNT =           "INVALID_AMOUNT";
         const char *NOT_ACTIVE_LIQUIDATION =   "NOT_ACTIVE_LIQUIDATION";
         const char *OPEN_LIQUIDATION =         "OPEN_LIQUIDATION";
         const char *NO_BID =                   "NO_BID";
         const char *INADEQUATE_BIDDING =       "INADEQUATE_BIDDING";
         const char *INSUFFICIENT_FUNDS =       "INSUFFICIENT_FUNDS";


         struct [[eosio::table]] config_table {
            uint64_t id;
            bool     pause;
            name     oracleAddress;
            name     liquidatorAdd;
            float    eosPrice;
            float    depositRate;
            uint64_t liquidationDuration;

            uint64_t primary_key() const { return id; }
         };


         struct [[eosio::table]] liquidation {
            uint64_t       loanid;
            asset          collateral;
            asset          amount;
            uint32_t       start_time;
            uint32_t       end_time;
            asset          best_bid;
            name           best_bider;
            uint8_t        state;

            uint64_t primary_key() const { return loanid; }
         };

         struct [[eosio::table]] deposit {
            name     user;
            asset    amount;

            uint64_t primary_key() const { return user.value; }
         };

         enum liqudiation_stats {
           ACTIVE = 0,
           FINISHED,
           FAILED
         }

         typedef eosio::multi_index< "config"_n, config_table > config;
         typedef eosio::multi_index< "deposits"_n, deposits_table > deposits;
         typedef eosio::multi_index< "trustfund"_n, trust_fund > trustfund;

         void init();
         void is_pausing();


   };

} /// namespace eosbank
