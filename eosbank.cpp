
#include <eosbank/eosbank.hpp>
#include <eosiolib/asset.hpp>

namespace eosio {

// eosbank::eosbank(  name        receiver,
//           name        code,
//           datastream  <const char*> ds):
//           contract( receiver, code, ds )
// {
//   init();
// }


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
            row.id = 0;
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


void bank::getloan( name user,
                    asset amount,
                    asset collateral)
{
    require_auth( user );

    enough_collateral( user, amount, collateral );
    // TODO: check for correct symbol

    // update the user used value of her/his collaterals
    trustfund fund( _code, _code.value );
    auto iterator = fund.find( user.value );
    fund.modify(iterator, _code, [&]( auto& row ) {
        row.used += collateral;
    });

    // submit a loan for user
    loan _loan( _code, _code.value );
    _loan.emplace(_code, [&]( auto& row ) {
        row.id = _loan.available_primary_key();
        row.debtor = user;
        row.collateralAmount = collateral;
        row.amount = amount;
        row.state = ACTIVE;
    });

    // issue token for user
    action(
        permission_level{ get_self(),"active"_n },
        "myteostoken"_n, // TODO: declare in config
        "issue"_n,
        std::make_tuple( user, amount, std::string("LOAN ISSUED") )
    ).send();
}


void bank::inccollatral( name user, uint64_t loanid, asset amount )
{
    require_auth(user);

    // check for loan exsisted
    loan _loan( _code, _code.value );
    auto iterator = _loan.find( loanid );
    eosio_assert ( iterator != _loan.end(), "LOAN NOT FOUND" );

    // check for loan owner and state
    const auto& item = _loan.get( loanid );
    eosio_assert ( item.id == loanid, ONLY_LOAN_OWNER );
    eosio_assert ( item.state == ACTIVE, NOT_ACTIVE_LOAN );

    // check for enoght collatral in trust fund
    trustfund fund( _code, _code.value );
    const auto& fund_item = fund.get( user.value );
    eosio_assert ( fund_item.fund >= (fund_item.used + amount), COLLATERAL_NOT_ENOUGH );// use diffrent error

    auto edit_fund = fund.find( user.value );
    fund.modify(edit_fund, _code, [&]( auto& row ) {
        row.used += amount;
    });

    // Add collatral to loan
    _loan.modify(iterator, _code, [&]( auto& row ) {
        row.collateralAmount += amount;
    });
}


void bank::enough_collateral( name user, asset amount, asset collateral ) {
    trustfund fund( _code, _code.value );
    const auto& item = fund.get( user.value );
    eosio_assert ( item.fund >= (item.used + collateral), COLLATERAL_NOT_ENOUGH );// use diffrent error

    config _config( _code, _code.value );
    const auto& cnf = _config.get( user.value );

    auto item_check = fund.find( user.value );
    eosio_assert ( item_check == fund.end(), "YOU HAVENT ANY ASSET" );
    eosio_assert ( (collateral.amount * cnf.eosPrice) >= (amount.amount * cnf.depositRate), COLLATERAL_NOT_ENOUGH);
}


} /// namespace eosbank


extern "C" {
    void apply( uint64_t receiver, uint64_t code, uint64_t action ) {
        if (action == "transfer"_n.value && code == "eosio.token"_n.value) {
            eosio::execute_action(eosio::name(receiver), eosio::name(code), &eosio::bank::geteos);
        }
        else if ( code == "eosbankzloan"_n.value ) { // code name should set in configs
            if( action == eosio::name( "chargeasset" ).value ) {
                execute_action( eosio::name(receiver), eosio::name(code), &eosio::bank::chargeasset );
            }
            else if( action == eosio::name( "setconfig" ).value ) {
                execute_action( eosio::name(receiver), eosio::name(code), &eosio::bank::setconfig );
            }
            else if( action == eosio::name( "getloan" ).value ) {
                execute_action( eosio::name(receiver), eosio::name(code), &eosio::bank::getloan );
            }
            else if( action == eosio::name( "inccollatral" ).value ) {
                execute_action( eosio::name(receiver), eosio::name(code), &eosio::bank::inccollatral );
            }
        }
    }
}


// EOSIO_DISPATCH( eosio::bank, (geteos))