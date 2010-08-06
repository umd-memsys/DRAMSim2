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






#ifndef BANK_H
#define BANK_H

//Bank.h
//
//Header file for bank class
//

#include "SystemConfiguration.h"
#include "SimulatorObject.h"
#include "BankState.h"
#include "BusPacket.h"

namespace DRAMSim
{
class Bank
{
	typedef struct _DataStruct
	{
		uint row;
		void *data;
		struct _DataStruct *next;
	} DataStruct;

public:
	//functions
	Bank();
	void read(BusPacket *busPacket);
	void write(const BusPacket *busPacket);

	//fields
	BankState currentState;

private:
	// private member
	std::vector<DataStruct *> rowEntries;

	static DataStruct *searchForRow(uint row, DataStruct *head);
};
}

#endif

