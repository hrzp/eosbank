
#include <oracles/oracles.hpp>
#include <eosiolib/asset.hpp>
#include <eosiolib/print.hpp>
#include <eosiolib/time.hpp>
#include <eosiolib/eosio.hpp>

using namespace eosio;

namespace eosio {


oracles::oracles(name receiver, name code, datastream<const char*> ds): contract(receiver, code, ds){
    init();
}


void oracles::init()
{

}


void oracles::setscore( name        account,
                        uint64_t    score)
{
    // TODO: add can recruiting
    require_auth( get_self() );
    oracles oracle( _code, _code.value );

    auto person = oracle.find( account.value );
    bool active_state = ( score > 0 ) ? true : false;

    // TODO: check total scores
    if ( person == oracle.end() ) {
        oracle.emplace(_code, [&]( auto& row ) {
            row.account     = account;
            row.score       = score;
            row.is_active   = active_state;
        });
    }
    else {
        oracle.modify(person, _code, [&]( auto& row ) {
            row.score = score;
            if ( active_state != row.is_active )
                row.is_active = active_state;
        });
    }
}


void oracles::vote( name        user,
                    uint8_t     type,
                    float       value)
{
    require_auth( user );
    oracle oracle( _code, _code.value );

    // TODO: check for correct type

    const auto& person = oracle.get( user.value, "No Account Founded" );
    eosio_assert ( person.is_active != false, "Not Active" );

    string u = std::to_string(user.value / 200) + std::to_string(type);
    uint64_t unique_id =  std::stoull(u);


    // should check user identifie for solidity
    votes vote( _code, _code.value );
    auto item = vote.find( unique_id );
    if ( item == vote.end() ) {
        vote.emplace(_code, [&]( auto& row ) {
            row.id = unique_id;
            row.type = type;
            row.sum = person.score * value;
            row.account_score = person.score;
        });
    }
    else {
        vote.modify(item, _code, [&]( auto& row ) {
            row.sum = person.score * value;
        });
    }

    uint64_t total_score = 0;
    for (auto itr = oracle.cbegin(); itr != oracle.cend(); itr++) {
        total_score += itr->score;
    }

    uint64_t sum = 0;
    uint64_t vote_scores = 0;
    for (auto itr = vote.cbegin(); itr != vote.cend(); itr++) {
        if ( itr->type != type )
            continue;
        sum += itr->sum;
        vote_scores += itr-> account_score;
    }

    if ( (total_score / vote_scores) < 2 )
        action(
            permission_level{ get_self(), "active"_n },
            "eosbank"_n,
            "setconfig"_n,
            std::make_tuple( type, float(sum / vote_scores) )
        ).send();
}

} /// namespace eosoracles



extern "C" {
    void apply(uint64_t receiver, uint64_t code, uint64_t action) {
        auto self = receiver;
        if( code == self ) {
          switch(action) {
            case name("vote").value:
              execute_action(name(receiver), name(code), &eosio::bank::vote);
              break;
            case name("setscore").value:
              execute_action(name(receiver), name(code), &eosio::bank::setscore);
              break;
          }
        }
    }
};
