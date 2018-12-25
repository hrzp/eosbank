// #pragma once

#include <eosiolib/asset.hpp>
#include <eosiolib/eosio.hpp>

#include <string>

namespace eosiosystem {
   class system_contract;
}

namespace eosio {

   using std::string;

   class [[eosio::contract("oracles")]] oracles : public contract {
      public:
         using contract::contract;

         oracles( name        receiver,
                  name        code,
                  datastream  <const char*> ds);

         [[eosio::action]]
         void withdraw( name user );

         [[eosio::action]]
         void startliq( name     eosbank,
                        uint64_t loanid,
                        asset    collatral,
                        asset    loan);

         [[eosio::action]]
         void stopliq( name user, uint64_t  liquidationid);

         [[eosio::action]]
         void palcebid( name user, uint64_t liquidationid, asset bid );

         [[eosio::action]]
         void getmyt( name   from,
                      name   to,
                      asset  quantity,
                      string memo);

         [[eosio::action]]
         void depositmyt( name   from,
                          asset  amount);


      private:
        #define MYT_SYMBOL symbol( "MYT", 4 )
        #define EOS_SYMBOL symbol( "EOS", 4 )
        const char *INVALID_ADDRESS =          "INVALID_ADDRESS";

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

        struct [[eosio::table]] recruiting_tb {
            uint64_t    id;
            bool        is_finished;
        };

        struct [[eosio::table]] vote_tb {
            uint64_t    id;
            uint64_t    voting_no;

            uint64_t primary_key() const { return id; }
        };

        struct [[eosio::table]] voting_tb {
            uint64_t    id;
            uint64_t    sum;
            uint64_t    sum_socres;
            uint64_t    no;

            uint64_t primary_key() const { return id; }
        };

        struct [[eosio::table]] oracle_tb {
            name        account;
            uint64_t    score;
            bool        is_active;

            uint64_t primary_key() const { return account.value; }
        };



        typedef eosio::multi_index< "config"_n, config_table > config;
        typedef eosio::multi_index< "vote"_n, vote_tb > vote;
        typedef eosio::multi_index< "voting"_n, voting_tb > voting;
        typedef eosio::multi_index< "oracles"_n, oracle_tb > oracles;
        typedef eosio::multi_index< "recruiting"_n, recruiting_tb > recruiting;

        void init();


   };

} /// namespace eosbank
