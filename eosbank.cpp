
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


void bank::setconfig( bool      pause,
                      name      oracleAddress,
                      name      liquidatorAdd,
                      float     eosPrice,
                      float     depositRate,
                      uint32_t  liquidationDuration)
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


void bank::geteos( name    from,
                   name    to,
                   asset   quantity,
                   string  memo)
{
    require_auth(from);

    if ( from == get_self() )
        return;

    #define EOS_SYMBOL symbol( "EOS", 4 )
    if ( quantity.symbol != EOS_SYMBOL )
        return;

    action(
        permission_level{ get_self(), "active"_n },
        get_self(),
        "chargeasset"_n,
        std::make_tuple( from, quantity )
    ).send();
}


void bank::chargeasset( name    from,
                        asset   quantity)
{
    require_auth( get_self() );

    trustfund fund( _code, _code.value );
    auto iterator = fund.find( from.value );
    if( iterator == fund.end() ) {
        fund.emplace(_code, [&]( auto& row ) {
            row.user = from;
            row.fund = quantity;
        });
    }
    else {
        fund.modify(iterator, _code, [&]( auto& row ) {
            row.fund += quantity;
        });
    }
}


} /// namespace eosbank


extern "C" {
    void apply( uint64_t receiver, uint64_t code, uint64_t action ) {
        if (action == "transfer"_n.value && code == "eosio.token"_n.value) {
            eosio::execute_action(name(receiver), name(code), &eosdoller::geteos);
        }
        else if ( code == "eosbankzloan"_n.value ) { // code name should set in configs
            if( action == name( "chargeasset" ).value ) {
                execute_action( name(receiver), name(code), &eosdoller::chargeasset );
            }
            else if( action == name( "setconfig" ).value ) {
                execute_action( name(receiver), name(code), &eosdoller::setconfig );
            }
        }
    }
}