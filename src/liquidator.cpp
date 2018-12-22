
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


void liquidator::withdraw( name user)
{
	require_auth( user );
	is_pausing();

    deposits deposit( _code, _code.value );

    const auto& item = deposit.get( user.value, NO_DEPOSIT );
    eosio_assert ( item.amount.amount > 0, NO_DEPOSIT);

    auto iterator = deposit.find( user.value );
    deposit.modify(iterator, _code, [&]( auto& row ) {
        row.amount = asset(0, MYT_SYMBOL);
    });

    // transfer eos for buyer
    action(
        permission_level{ get_self(),"active"_n },
        "myteostoken"_n, // TODO: declare in config
        "transfer"_n,
        std::make_tuple( get_self(), user, amount, std::string("WITHDRAW") )
    ).send();

}


} /// namespace eosliquidator


extern "C" {
    void apply( uint64_t receiver, uint64_t code, uint64_t action ) {

    }
}


// EOSIO_DISPATCH( eosio::liquidator, (geteos))