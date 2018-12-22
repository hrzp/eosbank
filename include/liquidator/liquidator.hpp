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

         void init();
         void is_pausing();


   };

} /// namespace eosbank
