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
        void setscore( name account, uint64_t score );

        [[eosio::action]]
        void vote( name user, uint8_t type, float value );

      private:
        #define EOD_SYMBOL symbol( "EOD", 4 )
        #define EOS_SYMBOL symbol( "EOS", 4 )
        const char *INVALID_ADDRESS =          "INVALID_ADDRESS";

        struct [[eosio::table]] config_table {
            uint64_t id;
            name     oracle_address;
            name     liquidator_add;
            float    eos_price;
            float    deposit_rate;
            float    liquidation_duration;

            uint64_t primary_key() const { return id; }
        };

        struct [[eosio::table]] recruiting_tb {
            uint64_t    id;
            bool        is_finished;
        };

        struct [[eosio::table]] vote_tb {
            uint64_t    id;
            uint8_t     type;
            float       sum;
            uint64_t    account_score;

            uint64_t primary_key() const { return id; }
        };

        struct [[eosio::table]] oracle_tb {
            name        account;
            uint64_t    score;
            bool        is_active;

            uint64_t primary_key() const { return account.value; }
        };

        typedef eosio::multi_index< "config"_n, config_table > config;
        typedef eosio::multi_index< "votes"_n, vote_tb > votes;
        typedef eosio::multi_index< "oracle"_n, oracle_tb > oracle;
        typedef eosio::multi_index< "recruiting"_n, recruiting_tb > recruiting;

        void init();


   };

} /// namespace eosbank
