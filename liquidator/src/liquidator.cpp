
#include <liquidator/liquidator.hpp>
#include <eosiolib/asset.hpp>
#include <eosiolib/print.hpp>
#include <eosiolib/time.hpp>
#include <eosiolib/eosio.hpp>

using namespace eosio;

namespace eosio {


liquidator::liquidator(name receiver, name code, datastream<const char*> ds): contract(receiver, code, ds){
    init();
}


void liquidator::init()
{

}


void liquidator::withdraw( name user)
{
	require_auth( user );

    deposits deposit( _code, _code.value );

    const auto& item = deposit.get( user.value, NO_DEPOSIT );
    eosio_assert ( 0 < item.amount.amount, NO_DEPOSIT);

    auto iterator = deposit.find( user.value );
    deposit.modify(iterator, _code, [&]( auto& row ) {
        row.amount = asset(0, EOD_SYMBOL);
    });

    action(
        permission_level{ get_self(),"active"_n },
        "eodtoken1111"_n, // TODO: declare in config
        "transfer"_n,
        std::make_tuple( get_self(), user, item.amount, std::string("WITHDRAW") )
    ).send();

}


void liquidator::startliq( name         eosbank,
				           uint64_t     loanid,
					       asset 	    collateral,
					       asset 	    loan)
{
    int min = 0;
	eosio_assert ( eosbank == "eosbank11111"_n, ONLY_EOS_BANK);
	eosio_assert ( min < collateral.amount, INVALID_AMOUNT );
	eosio_assert ( min < loan.amount, INVALID_AMOUNT );

    liquidations liquidation( _code, _code.value );
    config configs ( eosbank, eosbank.value ); // Read from eosbank

    const auto& bank = configs.get( 0 , "BANK NOT CONFIG YET");

    uint32_t start_time = now();
    uint32_t end_time = start_time + bank.liquidation_duration;

    liquidation.emplace(_code, [&]( auto& row ) {
        row.liquidationid   = liquidation.available_primary_key();
        row.loanid          = loanid;
        row.collateral      = collateral;
        row.amount          = loan;
        row.start_time      = start_time;
        row.end_time        = end_time;
        row.state           = ACTIVE;
    });
}


void liquidator::stopliq( name         user,
                          uint64_t     liquidationid)
{
    require_auth(user);
    liquidations liquidation( _code, _code.value );

    const auto& item = liquidation.get( liquidationid , "WRONG ID");
    eosio_assert ( now() > item.end_time, OPEN_LIQUIDATION );
    eosio_assert ( item.best_bid.amount != 0, NO_BID );
    eosio_assert ( item.state == ACTIVE, NOT_ACTIVE_LIQUIDATION );

    action(
        permission_level{ get_self(),"active"_n },
        "eodtoken1111"_n, // TODO: declare in config
        "transfer"_n,
        std::make_tuple( get_self(), "eosbank11111"_n, item.amount, std::string("FOR TOKEN BURN") )
    ).send();

    action(
        permission_level{ get_self(),"active"_n },
        "eosbank11111"_n, // TODO: declare in config
        "liquidated"_n,
        std::make_tuple( item.loanid, item.best_bid, item.best_bider )
    ).send();

    auto iterator = liquidation.find( liquidationid );
    liquidation.erase( iterator );
}



void liquidator::geteod( name    from,
                         name    to,
                         asset   quantity,
                         string  memo)
{
    require_auth(from);

    if ( from == get_self() )
        return;

    if ( quantity.symbol != EOD_SYMBOL )
        return;

    action(
        permission_level{ get_self(), "active"_n },
        get_self(),
        "depositeod"_n,
        std::make_tuple( from, quantity )
    ).send();
}



void liquidator::depositeod( name    from,
                             asset   amount)
{
    require_auth( get_self() );

    deposits deposit( _code, _code.value );
    auto iterator = deposit.find( from.value );
    if( iterator == deposit.end() ) {
        deposit.emplace(_code, [&]( auto& row ) {
            row.user    = from;
            row.amount  = amount;
        });
    }
    else {
        deposit.modify(iterator, _code, [&]( auto& row ) {
            row.amount += amount;
        });
    }
}


void liquidator::placebid( name         user,
                           uint64_t     liquidationid,
                           asset        bid)
{
    require_auth(user);

    liquidations liquidation( _code, _code.value );
    deposits deposit( _code, _code.value );
    eosio_assert ( bid.symbol == EOS_SYMBOL, "INVALID_EOS_SYMBOL" );

    const auto& item = liquidation.get( liquidationid , "WRONG ID");
    const auto& applicant = deposit.get( user.value , "NO FUND FOR BID");
    eosio_assert ( item.amount <= applicant.amount, INSUFFICIENT_FUNDS );
    eosio_assert ( item.state == ACTIVE, NOT_ACTIVE_LIQUIDATION );
    eosio_assert ( bid <= item.collateral, INADEQUATE_BIDDING );

    if ( item.best_bid.amount != 0 ) {
        eosio_assert ( bid < item.best_bid, INADEQUATE_BIDDING );
        auto depo = deposit.find( item.best_bider.value );
        deposit.modify(depo, _code, [&]( auto& row ) {
            row.amount += item.amount;
        });
    }
    auto depo = deposit.find( user.value );
    deposit.modify(depo, _code, [&]( auto& row ) {
        row.amount -= item.amount;
    });
    auto iterator = liquidation.find( liquidationid );
    liquidation.modify(iterator, _code, [&]( auto& row ){
        row.best_bider = user;
        row.best_bid = bid;
    });
}


} /// namespace eosliquidator


extern "C" {
    void apply(uint64_t receiver, uint64_t code, uint64_t action) {
        auto self = receiver;
        if( code == self ) {
          switch(action) {
            case name("startliq").value:
              execute_action(name(receiver), name(code), &eosio::liquidator::startliq);
              break;
            case name("stopliq").value:
              execute_action(name(receiver), name(code), &eosio::liquidator::stopliq);
              break;
            case name("withdraw").value:
              execute_action(name(receiver), name(code), &eosio::liquidator::withdraw);
              break;
            case name("placebid").value:
              execute_action(name(receiver), name(code), &eosio::liquidator::placebid);
              break;
            case name("depositeod").value:
              execute_action(name(receiver), name(code), &eosio::liquidator::depositeod);
              break;
          }
        }
        else if( code == "eodtoken1111"_n.value && action == "transfer"_n.value )
            eosio::execute_action(name(receiver), name(code), &eosio::liquidator::geteod);
    }
};