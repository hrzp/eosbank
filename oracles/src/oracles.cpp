
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
    oracle _oracle( _code, _code.value );

    auto person = _oracle.find( account.value );
    bool active_state = ( score > 0 ) ? true : false;

    // TODO: check total scores
    if ( person == _oracle.end() ) {
        _oracle.emplace(_code, [&]( auto& row ) {
            row.account     = account;
            row.score       = score;
            row.is_active   = active_state;
        });
    }
    else {
        _oracle.modify(person, _code, [&]( auto& row ) {
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
    oracle _oracle( _code, _code.value );

    // TODO: check for correct type

    const auto& person = _oracle.get( user.value, "No Account Founded" );
    eosio_assert ( person.is_active != false, "Not Active" );

    string u = std::to_string(user.value / 200) + std::to_string(type);
    uint64_t unique_id =  std::stoull(u);


    // should check user identifie for solidity
    votes _vote( _code, _code.value );
    auto item = _vote.find( unique_id );
    if ( item == _vote.end() ) {
        _vote.emplace(_code, [&]( auto& row ) {
            row.id = unique_id;
            row.type = type;
            row.sum = person.score * value;
            row.account_score = person.score;
        });
    }
    else {
        _vote.modify(item, _code, [&]( auto& row ) {
            row.sum = person.score * value;
        });
    }

    uint64_t total_score = 0;
    for (auto itr = _oracle.cbegin(); itr != _oracle.cend(); itr++) {
        total_score += itr->score;
    }

    uint64_t sum = 0;
    uint64_t vote_scores = 0;
    for (auto itr = _vote.cbegin(); itr != _vote.cend(); itr++) {
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
              execute_action(name(receiver), name(code), &eosio::oracles::vote);
              break;
            case name("setscore").value:
              execute_action(name(receiver), name(code), &eosio::oracles::setscore);
              break;
          }
        }
    }
};
