
#include <eosbank/eosbank.hpp>
#include <eosiolib/asset.hpp>
#include <eosiolib/print.hpp>

namespace eosio {


bank::bank(name receiver, name code, datastream<const char*> ds): contract(receiver, code, ds){
    // init();
}


void bank::init()
{
    config _config(_code, _code.value);
    auto iterator = _config.find( 0 );
    eosio_assert( iterator != _config.end(),  "CONFIG NOT SET");

    const auto& item = _config.get( 0 );
    // TODO: check config
}


void bank::initconfig( name     oracles,
                       name     liquidator,
                       float    eosPrice,
                       float    depositRate)
{
    require_auth( get_self() );
    config _config(_code, _code.value);
    auto iterator = _config.find( 0 );

    if( iterator == _config.end() ) {
        _config.emplace(_code, [&]( auto& row ) {
            row.id = 0;
            row.pause = false;
            row.oracleAddress = oracles;
            row.liquidatorAdd = liquidator;
            row.eosPrice = eosPrice;
            row.depositRate = depositRate;
            row.liquidationDuration = 3600;
        });
    }
}


void bank::setoracle( name address )
{
    require_auth( get_self() );
    config _config(_code, _code.value);
    auto iterator = _config.find( 0 );

    _config.modify(iterator, _code, [&]( auto& row ) {
        row.oracleAddress = address;
    });
}


void bank::setliqaddr( name address )
{
    require_auth( get_self() );
    config _config(_code, _code.value);
    auto iterator = _config.find( 0 );

    _config.modify(iterator, _code, [&]( auto& row ) {
        row.liquidatorAdd = address;
    });
}


void bank::setpause( bool value )
{
    require_auth( get_self() );
    config _config(_code, _code.value);
    auto iterator = _config.find( 0 );

    _config.modify(iterator, _code, [&]( auto& row ) {
        row.pause = value;
    });
}


void bank::setconfig( uint8_t   type,
                      float     value)
{
    require_auth("oracles"_n); // TODO: should read from config
    config _config(_code, _code.value);
    auto iterator = _config.find( 0 );

    _config.modify(iterator, _code, [&]( auto& row ) {
        switch (type) {
            case  EOS_PRICE:
                row.eosPrice = value;
                break;
            case DEPOSIT_RATE:
                row.depositRate = value;
                break;
            case LIQUIDATION_DURATION:
                row.liquidationDuration = value;
                break;
        }
    });
}


void bank::geteos( name    from,
                   name    to,
                   asset   quantity,
                   string  memo)
{
    require_auth(from);
    is_pausing();

    if ( from == get_self() )
        return;

    if ( quantity.symbol != EOS_SYMBOL )
        return;

    action(
        permission_level{ get_self(), "active"_n },
        get_self(),
        "depositeos"_n,
        std::make_tuple( from, quantity )
    ).send();
}


void bank::depositeos( name    from,
                        asset   quantity)
{
    require_auth( get_self() );
    is_pausing();

    trustfund fund( _code, _code.value );
    auto iterator = fund.find( from.value );
    if( iterator == fund.end() ) {
        fund.emplace(_code, [&]( auto& row ) {
            row.user = from;
            row.eos_fund = quantity;
        });
    }
    else {
        fund.modify(iterator, _code, [&]( auto& row ) {
            row.eos_fund += quantity;
        });
    }
}


void bank::getmyt( name    from,
                   name    to,
                   asset   quantity,
                   string  memo)
{
    require_auth(from);
    is_pausing();

    if ( from == get_self() )
        return;

    if ( quantity.symbol != MYT_SYMBOL )
        return;

    action(
        permission_level{ get_self(), "active"_n },
        get_self(),
        "depositmyt"_n,
        std::make_tuple( from, quantity )
    ).send();
}


void bank::depositmyt( name    from,
                       asset   amount)
{
    require_auth( get_self() );
    is_pausing();

    trustfund fund( _code, _code.value );
    auto iterator = fund.find( from.value );
    if( iterator == fund.end() ) {
        fund.emplace(_code, [&]( auto& row ) {
            row.user = from;
            row.myt_fund = amount;
        });
    }
    else {
        fund.modify(iterator, _code, [&]( auto& row ) {
            row.myt_fund += amount;
        });
    }
}


void bank::getloan( name user,
                    asset amount,
                    asset collateral)
{
    require_auth( user );
    is_pausing();

    enough_collateral( user, amount, collateral );
    // TODO: check for correct symbol

    // update the user used value of her/his collaterals
    trustfund fund( _code, _code.value );
    auto iterator = fund.find( user.value );
    fund.modify(iterator, _code, [&]( auto& row ) {
        row.eos_fund -= collateral;
    });

    // submit a loan for user
    loan _loan( _code, _code.value );
    _loan.emplace(_code, [&]( auto& row ) {
        row.id = _loan.available_primary_key();
        row.debtor = user;
        row.collateral_amount = collateral;
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


void bank::inccollatral( name user, uint64_t loanid, asset quantity )
{
    require_auth(user);
    is_pausing();

    loan _loan( _code, _code.value );

    // check for loan owner and state
    const auto& item = _loan.get( loanid, "LOAN NOT FOUND" );
    eosio_assert ( item.debtor == user, ONLY_LOAN_OWNER );
    eosio_assert ( item.state == ACTIVE, NOT_ACTIVE_LOAN );

    // check for enoght collatral in trust fund
    trustfund fund( _code, _code.value );
    const auto& fund_item = fund.get( user.value );
    eosio_assert ( fund_item.eos_fund >=  quantity, COLLATERAL_NOT_ENOUGH );

    auto edit_fund = fund.find( user.value );
    fund.modify(edit_fund, _code, [&]( auto& row ) {
        row.eos_fund -= quantity;
    });

    // Add collatral to loan
    auto iterator = _loan.find( loanid );
    _loan.modify(iterator, _code, [&]( auto& row ) {
        row.collateral_amount += quantity;
    });
}


void bank::settleloan( name user, uint64_t loanid, asset amount )
{
    require_auth(user);
    is_pausing();

    // check for loan exsisted
    loan _loan( _code, _code.value );
    auto iterator = _loan.find( loanid );

    // check for loan owner and state
    const auto& item = _loan.get( loanid, "LOAN NOT FOUND" );
    eosio_assert ( item.state == ACTIVE, NOT_ACTIVE_LOAN );
    eosio_assert ( item.amount <= amount, INVALID_AMOUNT );

    trustfund fund( _code, _code.value );
    const auto& fund_item = fund.get( user.value, "YOU HAVENT ANY ASSET" );
    eosio_assert ( fund_item.myt_fund >= amount, COLLATERAL_NOT_ENOUGH );
    auto fund_itr = fund.find( user.value );
    fund.modify(fund_itr, _code, [&]( auto& row ) {
        row.myt_fund -= amount;
    });

    float payback = item.collateral_amount.amount * amount.amount / item.amount.amount;
    asset paybackEOS = asset(payback, EOS_SYMBOL);

    action(
        permission_level{ get_self(),"active"_n },
        "myteostoken"_n, // TODO: declare in config
        "retire"_n,
        std::make_tuple( amount, std::string("LOAN BURN") )
    ).send();

    _loan.modify(iterator, _code, [&]( auto& row ) {
        row.collateral_amount -= paybackEOS;
        row.amount -= amount;
        if ( row.amount.amount == 0 )
            row.state = SETTLED;
    });

    // transfer eos for user
    action(
        permission_level{ get_self(),"active"_n },
        "eosio.token"_n, // TODO: declare in config
        "transfer"_n,
        std::make_tuple( get_self(), item.debtor, paybackEOS, std::string("LOAN ISSUED") )
    ).send();
}


void bank::liquidate(name user, uint64_t loanid)
{
    require_auth(user);
    is_pausing();

    // check for loan exsisted
    loan _loan( _code, _code.value );

    const auto& item = _loan.get( loanid, "LOAN NOT FOUND" );

    config _config( _code, _code.value );
    const auto& cnf = _config.get( 0 );
    eosio_assert ( item.state == ACTIVE, NOT_ACTIVE_LOAN);
    eosio_assert( (item.collateral_amount.amount * cnf.eosPrice) < (item.amount.amount * cnf.depositRate), ENOUGH_COLLATERAL );

    auto iterator = _loan.find( loanid );
    _loan.modify(iterator, _code, [&]( auto& row ) {
        row.state = UNDER_LIQUIDATION;
    });

    action(
        permission_level{ get_self(),"active"_n },
        "liquidator"_n, // TODO: declare in config
        "startliquidat"_n,
        std::make_tuple( get_self(), loanid, item.collateral_amount, item.amount )
    ).send();

}


void bank::liquidated( uint64_t loanid, asset amount, name buyer )
{
    require_auth("onlyLiquidator"_n);
    is_pausing();

    // check for loan exsisted
    loan _loan( _code, _code.value );

    const auto& item = _loan.get( loanid, "LOAN NOT FOUND" );
    auto iterator = _loan.find( loanid );
    _loan.modify(iterator, _code, [&]( auto& row ) {
        row.state = LIQUIDATED;
        row.amount = asset(0, MYT_SYMBOL);
        row.collateral_amount -= amount;
    });

    // transfer eos for buyer
    action(
        permission_level{ get_self(),"active"_n },
        "eosio.token"_n, // TODO: declare in config
        "transfer"_n,
        std::make_tuple( get_self(), buyer, amount, std::string("LOAN LIQUIDATED") )
    ).send();

    // burn the tokens
    action(
        permission_level{ get_self(),"active"_n },
        "myteostoken"_n, // TODO: declare in config
        "retire"_n,
        std::make_tuple( amount, std::string("LOAN BURN") )
    ).send();
}


void bank::enough_collateral( name user, asset amount, asset collateral ) {
    trustfund fund( _code, _code.value );
    const auto& item = fund.get( user.value, "YOU HAVENT ANY ASSET" );
    eosio_assert ( item.eos_fund >= collateral, COLLATERAL_NOT_ENOUGH );

    config _config( _code, _code.value );
    const auto& cnf = _config.get( 0 );
    eosio_assert ( (collateral.amount * cnf.eosPrice) >= (amount.amount * cnf.depositRate), COLLATERAL_NOT_ENOUGH);
}


void bank::is_pausing()
{
    config _config(_code, _code.value);
    const auto& config = _config.get( 0 );
    eosio_assert( config.pause == false,  "CONTRACT PAUSED.");
}


} /// namespace eosbank


extern "C" {
    void apply( uint64_t receiver, uint64_t code, uint64_t action ) {
        if (action == "transfer"_n.value && code == "eosio.token"_n.value) {
            eosio::execute_action(eosio::name(receiver), eosio::name(code), &eosio::bank::geteos);
        }
        else if (action == "transfer"_n.value && code == "myteostoken"_n.value){
            if( action == eosio::name( "getmyt" ).value ) {
                execute_action( eosio::name(receiver), eosio::name(code), &eosio::bank::getmyt );
            }
        }
        else if ( code == "eosbank"_n.value ) { // code name should set in configs
            if( action == eosio::name( "depositeos" ).value ) {
                execute_action( eosio::name(receiver), eosio::name(code), &eosio::bank::depositeos );
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
            else if( action == eosio::name( "depositmyt" ).value ) {
                execute_action( eosio::name(receiver), eosio::name(code), &eosio::bank::depositmyt );
            }
            else if( action == eosio::name( "liquidate" ).value ) {
                execute_action( eosio::name(receiver), eosio::name(code), &eosio::bank::liquidate );
            }
        }
    }
}


// EOSIO_DISPATCH( eosio::bank, (geteos))