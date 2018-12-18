#include <eosiolib/asset.hpp>
#include <eosiolib/eosio.hpp>

#include <string>


namespace eosbank {

   using std::string;

   class [[eosio::contract("eosbank")]] eosbankcontract : public contract {
      public:
         using contract::contract;

      private:

   };

} /// namespace eosbank
