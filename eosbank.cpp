
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
    asset myt = asset( amount, MYT_SYMBOL );


    // update the user used value of her/his collaterals
    trust_fund trustfund( _code, _code.value );
    auto iterator = trustfund.find( from.value );
    trust_fund.modify(iterator, _code, [&]( auto& row ) {
        row.used += collateral;
    });

    // submit a loan for user
    loan _loan( _code, _code.value );
    _loan.emplace(_code, [&]( auto& row ) {
        row.id = _loan.available_primary_key();
        row.debtor = user;
        row.collateralAmount = collateral;
        row.amount = myt;
        row.state = LoanState.ACTIVE;
    });

    // issue token for user
    action(
        permission_level{ get_self(),"active"_n },
        "myteostoken"_n, // TODO: declare in config
        "issue"_n,
        std::make_tuple( user, myt, std::string("LOAN ISSUED") )
    ).send();
}


void bank::increasecollatral( name user, uint64_t loanid, asset amount )
{
    require_auth(user);

    // check for loan exsisted
    loan _loan( _code, _code.value );
    auto iterator = _loan.find( loanid );
    eosio_assert ( iterator != _loan.end(), "LOAN NOT FOUND" );

    // check for loan owner and state
    const auto& item = _loan.get( loanid );
    eosio_assert ( item.id == loanid, ONLY_LOAN_OWNER )
    eosio_assert ( item.state == LoanState.ACTIVE, NOT_ACTIVE_LOAN );

    // check for enoght collatral in trust fund
    trustfund fund( _code, _code.value );
    const auto& fund_item = fund.get( user.value );
    eosio_assert ( fund_item.asset >= (fund_item.used + amount), COLLATERAL_NOT_ENOUGH );// use diffrent error

    auto edit_fund = fund.find( user.value );
    fund.modify(edit_fund, _code, [&]( auto& row ) {
        row.used += amount;
    });

    // Add collatral to loan
    _loan.modify(iterator, _code, [&]( auto& row ) {
        row.collateralAmount += amount;
    });
}


bool bank::enough_collateral( name user, asset amount, asset collateral ) {
    trustfund fund( _code, _code.value );
    const auto& item = fund.get( user.value );
    eosio_assert ( item.asset >= (item.used + collateral), COLLATERAL_NOT_ENOUGH );// use diffrent error

    config _config( _code, _code.value );
    const auto& cnf = _config.get( user.value );

    eosio_assert ( item == fund.end(), "YOU HAVENT ANY ASSET" );
    eosio_assert ( (collateral.amount * cnf.eosPrice) >= (amount * cnf.depositRate), COLLATERAL_NOT_ENOUGH);
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
            else if( action == name( "getloan" ).value ) {
                execute_action( name(receiver), name(code), &eosdoller::getloan );
            }
        }
    }
}