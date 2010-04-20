//BankState.cpp
//
//Class file for bank state object
//

#include "BankState.h"

using namespace std;
using namespace DRAMSim;

//All banks start precharged
BankState::BankState():
	currentBankState(Idle),
	openRowAddress(0),
	nextRead(0),
	nextWrite(0),
	nextActivate(0),
	nextPrecharge(0),
	nextPowerUp(0),
	lastCommand(READ),
	stateChangeCountdown(0)
{}

void BankState::print()
{
	PRINT(" == Bank State ");
	if(currentBankState == Idle)
		{
			PRINT("    State : Idle" );
		}
	else if(currentBankState == RowActive)
		{
			PRINT("    State : Active" );
		}
	else if(currentBankState == Refreshing)
		{
			PRINT("    State : Refreshing" );
		}
	else if(currentBankState == PowerDown)
		{
			PRINT("    State : Power Down" );
		}

	PRINT("    OpenRowAddress : " << openRowAddress );
	PRINT("    nextRead       : " << nextRead );
	PRINT("    nextWrite      : " << nextWrite );
	PRINT("    nextActivate   : " << nextActivate );
	PRINT("    nextPrecharge  : " << nextPrecharge );
	PRINT("    nextPowerUp    : " << nextPowerUp );
}
