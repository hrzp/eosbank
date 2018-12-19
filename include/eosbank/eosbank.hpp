#include <eosiolib/asset.hpp>
#include <eosiolib/eosio.hpp>

#include <string>


namespace eosbank {

   using std::string;

   class [[eosio::contract("eosbank")]] bank : public contract {
      public:
         using contract::contract;

         [[eosio::action]]
         void setconfig( bool      pause,
                         name      oracleAddress,
                         name      liquidatorAdd,
                         float     eosPrice,
                         float     depositRate,
                         uint32_t  LIQUIDATION_DURATION);

         [[eosio::action]]
         void chargeasset( name   from,
                           asset  quantity);

         [[eosio::action]]
         void geteos( name   from,
                      name   to,
                      asset  quantity,
                      string memo);

         [[eosio::action]]
         void getloan( name   user,
                      float  quantity);

         [[eosio::action]]
         void increasecollatral( name user,
                                 uint64_t loanid,
                                 asset amount);


      private:
         #define MYT_SYMBOL symbol( "MYT", 4 )
         #define EOS_SYMBOL symbol( "EOS", 4 )
         float eosPrice; // for global usage
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
            bool     pause;
            name     oracleAddress;
            name     liquidatorAdd;
            float    eosPrice;
            float    depositRate;
            uint64_t liquidationDuration; // better to be in uint32

            uint64_t primary_key() const { return id; }
         };

         struct [[eosio::table]] loan_table {
            uint64_t id;
            name     debtor;
            asset    collateralAmount;
            asset    amount;
            uint8_t  state;

            uint64_t primary_key() const { return id; }
         };

         struct [[eosio::table]] trust_fund {
            name     user;
            asset    fund;
            asset    used;

            uint64_t primary_key() const { return user.value; }
         };

         typedef eosio::multi_index< "config"_n, config_table > config;
         typedef eosio::multi_index< "loan"_n, loan_table > loan;
         typedef eosio::multi_index< "trustfund"_n, trust_fund > trustfund;

         enum LoanState {
            ACTIVE,
            UNDER_LIQUIDATION,
            LIQUIDATED,
            SETTLED
         };

         enum Types {
            EOS_PRICE,
            DEPOSIT_RATE,
            LIQUIDATION_DURATION
         }

         void init();
         void is_pausing();
         void enough_collateral( name user, float amount );

   };

} /// namespace eosbank
