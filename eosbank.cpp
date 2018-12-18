
#include <eosbank/eosbank.hpp>

namespace eosbank {

eosbank(  name        receiver,
          name        code,
          datastream  <const char*> ds):
          contract( receiver, code, ds )
{
  init();
}


void bank::init()
{
    config _config(_code, _code.value);
    auto iterator = _config.find( 0 );
    eosio_assert( iterator != _config.end(),  "CONFIG NOT SET");

    const auto& item = _config.get( 0 );
    bank::eosPrice = item.eosPrice;
    // TODO: check config
}


void bank::is_pausing()
{
    config _config(_code, _code.value);
    const auto& config = _config.get( 0 );
    eosio_assert( config.pause == false,  "CONTRACT PAUSED.");
}

void bank::set_config(  bool    pause,
                        name    oracleAddress,
                        name    liquidatorAdd,
                        float   eosPrice,
                        float   depositRate,
                        uint32_t liquidationDuration)
{
    require_auth(get_self());
    config _config(_code, _code.value);
    auto iterator = _config.find( 0 );
    if( iterator == _config.end() ) {
        _config.emplace(_code, [&]( auto& row ) {
            row.key = 0;
            row.pause = pause;
            row.oracleAddress = oracleAddress;
            row.liquidatorAdd = liquidatorAdd;
            row.eosPrice = eosPrice;
            row.depositRate = depositRate;
            row.liquidationDuration = liquidationDuration;
        });
    }
    else {
        _config.modify(iterator, _code, [&]( auto& row ) {
            row.pause = pause;
            row.oracleAddress = oracleAddress;
            row.liquidatorAdd = liquidatorAdd;
            row.eosPrice = eosPrice;
            row.depositRate = depositRate;
            row.liquidationDuration = liquidationDuration;
        });
    }
}

} /// namespace eosbank
