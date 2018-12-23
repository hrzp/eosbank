
#include <liquidator/liquidator.hpp>
#include <eosiolib/asset.hpp>
#include <eosiolib/print.hpp>
#include <eosiolib/time.hpp>

namespace eosio {


liq::liq(name receiver, name code, datastream<const char*> ds): contract(receiver, code, ds){
    // init();
}


void liq::init()
{

}


void liq::is_pausing()
{

}


void liq::withdraw( name user)
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
        std::make_tuple( get_self(), user, item.amount, std::string("WITHDRAW") )
    ).send();

}


void liquidator::startliq( name 	eosbank,
						   uint64_t loanid,
						   asset 	collatral,
						   asset 	loan)
{
	eosio_assert ( eosbank == "eosbank"_n, ONLY_EOS_BANK);
	eosio_assert( collatral.amount > 0, INVALID_AMOUNT );
	eosio_assert( loan.amount > 0, INVALID_AMOUNT );

    liquidations liquidation( _code, _code.value );
    config configs ( eosbank, eosbank.value ); // Read from eosbank

    const auto& bank = configs.get( 0 , "BANK NOT CONFIG YET");

    auto iterator = liquidation.find( loanid );
    eosio_assert ( iterator == liquidation.end(), "Duplicated loan id for liquidation" );

    uint32_t start_time = now();
    uint32_t end_time = start_time + bank.liquidationDuration;

    liquidation.emplace(_code, [&]( auto& row ) {
        row.loanid = loanid;
        row.collatral = collatral;
        row.amount = amount;
        row.start_time = start_time;
        row.end_time = end_time;
        row.state = ACTIVE;
    });
}

} /// namespace eosliquidator


extern "C" {
    void apply( uint64_t receiver, uint64_t code, uint64_t action ) {
        if (action == "transfer"_n.value && code == "eosio.token"_n.value) {
            // eosio::execute_action(eosio::name(receiver), eosio::name(code), &eosio::bank::geteos);
        }
        else if ( code == "liquidator"_n.value ) { // code name should set in configs
            if( action == eosio::name( "startliq" ).value ) {
                execute_action( eosio::name(receiver), eosio::name(code), &eosio::liq::startliq );
            }
            else if( action == eosio::name( "withdraw" ).value ) {
                execute_action( eosio::name(receiver), eosio::name(code), &eosio::liq::withdraw );
            }
        }
    }
}


// EOSIO_DISPATCH( eosio::liquidator, (geteos))