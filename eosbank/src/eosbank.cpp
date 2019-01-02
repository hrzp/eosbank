
#include <eosbank/eosbank.hpp>
#include <eosiolib/asset.hpp>
#include <eosiolib/print.hpp>
#include <eosiolib/eosio.hpp>

using namespace eosio;

namespace eosio {


bank::bank(name receiver, name code, datastream<const char*> ds): contract(receiver, code, ds){
    init();
}


void bank::init()
{

}


void bank::initconfig( name     oracles,
                       name     liquidator,
                       float    eos_price,
                       float    collateral_ratio,
                       float    liquidation_duration)
{
    require_auth( get_self() );
    config _config(_code, _code.value);
    auto iterator = _config.find( 0 );

    if( iterator != _config.end() ) {
        return;
    };
    _config.emplace(_code, [&]( auto& row ) {
        row.id = 0;
        row.oracle_address = oracles;
        row.liquidator_add = liquidator;
        row.eos_price = eos_price;
        row.collateral_ratio = collateral_ratio;
        row.liquidation_duration = liquidation_duration; // in second
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
                row.eos_price = value;
                break;
            case COLLATERAL_RATIO:
                row.collateral_ratio = value;
                break;
            case LIQUIDATION_DURATION:
                row.liquidation_duration = value;
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

    if ( from == get_self() )
        return;

    if ( quantity.symbol != EOS_SYMBOL ) // it just for testnet
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


void bank::geteod( name    from,
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


void bank::depositeod( name    from,
                       asset   amount)
{
    require_auth( get_self() );

    trustfund fund( _code, _code.value );
    auto iterator = fund.find( from.value );
    if( iterator == fund.end() ) {
        fund.emplace(_code, [&]( auto& row ) {
            row.user = from;
            row.eod_fund = amount;
        });
    }
    else {
        fund.modify(iterator, _code, [&]( auto& row ) {
            row.eod_fund += amount;
        });
    }
}


void bank::getloan( name    user,
                    asset   amount,
                    asset   collateral)
{
    require_auth( user );

    // TODO: check for correct symbol
    enough_collateral( user, amount, collateral );

    trustfund fund( _code, _code.value );
    auto iterator = fund.find( user.value );
    fund.modify(iterator, _code, [&]( auto& row ) {
        row.eos_fund -= collateral;
    });

    // submit a loan for user
    loan _loan( _code, _code.value );
    _loan.emplace(_code, [&]( auto& row ) {
        row.id = _loan.available_primary_key();
        row.debtor              = user;
        row.collateral_amount   = collateral;
        row.amount              = amount;
        row.state               = ACTIVE;
    });

    // issue token for user
    action(
        permission_level{ get_self(),"active"_n },
        "eodtoken1111"_n, // TODO: declare in config
        "issue"_n,
        std::make_tuple( user, amount, std::string("LOAN ISSUED") )
    ).send();
}


void bank::incrsclltrl( name user,
                        uint64_t loanid,
                        asset quantity)
{
    require_auth(user);

    loan _loan( _code, _code.value );

    const auto& item = _loan.get( loanid, "LOAN NOT FOUND" );
    eosio_assert ( item.debtor == user, ONLY_LOAN_OWNER );
    eosio_assert ( item.state == ACTIVE, NOT_ACTIVE_LOAN );

    trustfund fund( _code, _code.value );
    const auto& fund_item = fund.get( user.value );
    eosio_assert ( fund_item.eos_fund >=  quantity, COLLATERAL_NOT_ENOUGH );

    auto assets = fund.find( user.value );
    fund.modify(assets, _code, [&]( auto& row ) {
        row.eos_fund -= quantity;
    });

    auto iterator = _loan.find( loanid );
    _loan.modify(iterator, _code, [&]( auto& row ) {
        row.collateral_amount += quantity;
    });
}


void bank::settleloan( name         user,
                       uint64_t     loanid,
                       asset        amount)
{
    require_auth(user);

    loan _loan( _code, _code.value );
    auto iterator = _loan.find( loanid );

    const auto& item = _loan.get( loanid, "LOAN NOT FOUND" );
    eosio_assert ( item.state == ACTIVE, NOT_ACTIVE_LOAN );
    eosio_assert ( item.amount <= amount, INVALID_AMOUNT );

    trustfund fund( _code, _code.value );
    const auto& fund_item = fund.get( user.value, "YOU HAVENT ANY ASSET" );
    eosio_assert ( fund_item.eod_fund >= amount, COLLATERAL_NOT_ENOUGH );
    auto fund_itr = fund.find( user.value );
    fund.modify(fund_itr, _code, [&]( auto& row ) {
        row.eod_fund -= amount;
    });

    float payback = item.collateral_amount.amount * amount.amount / item.amount.amount;
    asset paybackEOS = asset(payback, EOS_SYMBOL);

    action(
        permission_level{ get_self(),"active"_n },
        "eodtoken1111"_n, // TODO: declare in config
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


void bank::liquidate( name          user,
                      uint64_t      loanid)
{
    require_auth(user);

    loan _loan( _code, _code.value );
    const auto& item = _loan.get( loanid, "LOAN NOT FOUND" );

    config _config( _code, _code.value );
    const auto& cnf = _config.get( 0 );
    eosio_assert ( item.state == ACTIVE, NOT_ACTIVE_LOAN);
    eosio_assert( (item.collateral_amount.amount * cnf.eos_price) < (item.amount.amount * cnf.collateral_ratio), ENOUGH_COLLATERAL );

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


void bank::liquidated( uint64_t     loanid,
                       asset        amount,
                       name         winer)
{
    require_auth("liquidator"_n); // TODO: should read from config

    loan _loan( _code, _code.value );

    const auto& item = _loan.get( loanid, "LOAN NOT FOUND" );
    auto iterator = _loan.find( loanid );
    _loan.modify(iterator, _code, [&]( auto& row ) {
        row.state = LIQUIDATED;
        row.amount = asset(0, EOD_SYMBOL);
        row.collateral_amount -= amount;
    });

    action(
        permission_level{ get_self(),"active"_n },
        "eosio.token"_n, // TODO: declare in config
        "transfer"_n,
        std::make_tuple( get_self(), winer, amount, std::string("LOAN LIQUIDATED") )
    ).send();

    // burn the tokens
    action(
        permission_level{ get_self(),"active"_n },
        "eodtoken1111"_n, // TODO: declare in config
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
    eosio_assert ( (collateral.amount * cnf.eos_price) >= (amount.amount * cnf.collateral_ratio), COLLATERAL_NOT_ENOUGH);
}


void bank::reset()
{
    // just for test
    require_auth( get_self() );
    config _config(_code, _code.value);
    auto iterator = _config.find( 0 );

    _config.erase(iterator);

}


} /// namespace eosbank


extern "C" {
    void apply(uint64_t receiver, uint64_t code, uint64_t action) {
        auto self = receiver;
        if( code == self ) {
          switch(action) {
            case name("depositeos").value:
              execute_action(name(receiver), name(code), &eosio::bank::depositeos);
              break;
            case name("setconfig").value:
              execute_action(name(receiver), name(code), &eosio::bank::setconfig);
              break;
            case name("reset").value:
              execute_action(name(receiver), name(code), &eosio::bank::reset);
              break;
            case name("getloan").value:
              execute_action(name(receiver), name(code), &eosio::bank::getloan);
              break;
            case name("incrsclltrl").value:
              execute_action(name(receiver), name(code), &eosio::bank::incrsclltrl);
              break;
            case name("liquidate").value:
              execute_action(name(receiver), name(code), &eosio::bank::liquidate);
              break;
            case name("depositeod").value:
              execute_action(name(receiver), name(code), &eosio::bank::depositeod);
              break;
            case name("initconfig").value:
              execute_action(name(receiver), name(code), &eosio::bank::initconfig);
              break;
          }
        }
        else if( code == "eosio.token"_n.value && action == "transfer"_n.value )
            eosio::execute_action(name(receiver), name(code), &eosio::bank::geteos);
        else if( code == "eodtoken1111"_n.value && action == "transfer"_n.value )
            execute_action( name(receiver), name(code), &eosio::bank::geteod );
    }
};


// EOSIO_DISPATCH( eosio::bank, (geteos))