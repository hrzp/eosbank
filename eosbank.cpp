
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
    // TODO: check config
}


void is_pausing()
{
    config _config(_code, _code.value);
    const auto& config = _config.get( 0 );
    eosio_assert( config.pause == false,  "CONTRACT PAUSED.");
}

} /// namespace eosbank
