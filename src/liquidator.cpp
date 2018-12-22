
#include <eosbank/liquidator.hpp>
#include <eosiolib/asset.hpp>
#include <eosiolib/print.hpp>

namespace eosio {


liquidator::liquidator(name receiver, name code, datastream<const char*> ds): contract(receiver, code, ds){
    // init();
}


void liquidator::init()
{

}


void liquidator::is_pausing()
{

}


} /// namespace eosliquidator


extern "C" {
    void apply( uint64_t receiver, uint64_t code, uint64_t action ) {

    }
}


// EOSIO_DISPATCH( eosio::liquidator, (geteos))