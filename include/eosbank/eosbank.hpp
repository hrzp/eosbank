#include <eosiolib/asset.hpp>
#include <eosiolib/eosio.hpp>

#include <string>


namespace eosbank {

   using std::string;

   class [[eosio::contract("eosbank")]] bank : public contract {
      public:
         using contract::contract;

      private:
         struct [[eosio::table]] config_table {
            uint64_t id;
            bool     pause;
            name     oracleAddress;
            name     liquidatorAdd;
            float    eosPrice;
            float    depositRate;
            uint64_t liquidationDuration; // better to be in uint32

            uint64_t primary_key() const { return id; }
         };

         struct [[eosio::table]] loan {
            uint64_t id;
            name     debtor;
            asset    collateralAmount;
            asset    amount;
            uint8_t  state;

            uint64_t primary_key() const { return debtor.value; }
         };

         typedef eosio::multi_index< "config"_n, config_table > config;

         void init();
         void is_pausing();
   };

} /// namespace eosbank
