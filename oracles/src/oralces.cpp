
#include <liquidator/liquidator.hpp>
#include <eosiolib/asset.hpp>
#include <eosiolib/print.hpp>
#include <eosiolib/time.hpp>

namespace eosio {


oracles::oracles(name receiver, name code, datastream<const char*> ds): contract(receiver, code, ds){
    // init();
    // TODO: check for configs
}


void oracles::init()
{

}


void oracles::setscore( name account, uint64_t score )
{
    // TODO: add can recruiting and is pause
    require_auth( get_self() );
    oracles oracle( _code, _code.value );

    auto person = oracle.find( account.value );
    bool active_state = ( score > 0 ) ? true : false;

    // TODO: check total scores
    if ( person == oracle.end() ) {
        oracle.emplace(_code, [&]( auto& row ) {
            row.account = account;
            row.score = score;
            row.is_active = active_state;
        });
    }
    else {
        oracle.modify(person, _code, [&]( auto& row ) {
            row.score = score;
            if ( active_state != row.is_active )
                row.is_active = active_state;
        });
    }
    //     for (auto itr = deposit.cbegin(); itr != deposit.cend(); itr++) {
    //       print( itr->amount, "  " );
    // }
}

} /// namespace eosliquidator


extern "C" {
    void apply( uint64_t receiver, uint64_t code, uint64_t action ) {
        if (action == "transfer"_n.value && code == "myteostoken"_n.value) {
            eosio::execute_action(eosio::name(receiver), eosio::name(code), &eosio::liq::getmyt);
        }
        else if ( code == "liquidator"_n.value ) { // code name should set in configs
            if( action == eosio::name( "startliq" ).value ) {
                execute_action( eosio::name(receiver), eosio::name(code), &eosio::liq::startliq );
            }
            else if( action == eosio::name( "withdraw" ).value ) {
                execute_action( eosio::name(receiver), eosio::name(code), &eosio::liq::withdraw );
            }
            else if( action == eosio::name( "palcebid" ).value ) {
                execute_action( eosio::name(receiver), eosio::name(code), &eosio::liq::palcebid );
            }
            else if( action == eosio::name( "depositmyt" ).value ) {
                execute_action( eosio::name(receiver), eosio::name(code), &eosio::liq::depositmyt );
            }
        }
    }
}
