/****************************************************************************
*	 DRAMSim2: A Cycle Accurate DRAM simulator 
*	 
*	 Copyright (C) 2010   	Elliott Cooper-Balis
*									Paul Rosenfeld 
*									Bruce Jacob
*									University of Maryland
*
*	 This program is free software: you can redistribute it and/or modify
*	 it under the terms of the GNU General Public License as published by
*	 the Free Software Foundation, either version 3 of the License, or
*	 (at your option) any later version.
*
*	 This program is distributed in the hope that it will be useful,
*	 but WITHOUT ANY WARRANTY; without even the implied warranty of
*	 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*	 GNU General Public License for more details.
*
*	 You should have received a copy of the GNU General Public License
*	 along with this program.  If not, see <http://www.gnu.org/licenses/>.
*
*****************************************************************************/






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
	if (currentBankState == Idle)
	{
		PRINT("    State : Idle" );
	}
	else if (currentBankState == RowActive)
	{
		PRINT("    State : Active" );
	}
	else if (currentBankState == Refreshing)
	{
		PRINT("    State : Refreshing" );
	}
	else if (currentBankState == PowerDown)
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
