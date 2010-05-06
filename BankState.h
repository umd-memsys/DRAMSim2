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






#ifndef BANKSTATE_H
#define BANKSTATE_H

//BankState.h
//
//Header file for bank state class
//

#include "SystemConfiguration.h"
#include "BusPacket.h"

namespace DRAMSim
{
enum CurrentBankState
{
	Idle,
	RowActive,
	Precharging,
	Refreshing,
	PowerDown
};

class BankState
{
public:
	//Fields
	CurrentBankState currentBankState;
	uint openRowAddress;
	uint64_t nextRead;
	uint64_t nextWrite;
	uint64_t nextActivate;
	uint64_t nextPrecharge;
	uint64_t nextPowerUp;

	BusPacketType lastCommand;
	uint stateChangeCountdown;

	//Functions
	BankState();
	void print();
};
}

#endif

