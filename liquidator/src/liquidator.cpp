
#include <liquidator/liquidator.hpp>
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
}

    // function startLiquidation(
    //     uint256 _loanId,
    //     uint256 _collateralAmount,
    //     uint256 _loanAmount
    // )
    //     external
    //     whenNotPaused
    //     onlyEtherBankSC
    //     throwIfEqualToZero(_collateralAmount)
    //     throwIfEqualToZero(_loanAmount)
    // {
    //     uint256 startTime = now;
    //     uint256 endTime = startTime + bank.liquidationDuration();
    //     uint256 liquidationId = ++lastLiquidationId;
    //     liquidations[liquidationId].loanId = _loanId;
    //     liquidations[liquidationId].collateralAmount = _collateralAmount;
    //     liquidations[liquidationId].loanAmount = _loanAmount;
    //     liquidations[liquidationId].startTime = startTime;
    //     liquidations[liquidationId].endTime = endTime;
    //     liquidations[liquidationId].state = LiquidationState.ACTIVE;
    //     emit StartLiquidation(liquidationId, _loanId, _collateralAmount, _loanAmount, startTime, endTime);
    // }



} /// namespace eosliquidator


extern "C" {
    void apply( uint64_t receiver, uint64_t code, uint64_t action ) {

    }
}


// EOSIO_DISPATCH( eosio::liquidator, (geteos))