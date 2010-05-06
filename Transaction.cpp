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






//Transaction.cpp
//
//Class file for transaction object
//	Transaction is considered requests sent from the CPU to
//	the memory controller (read, write, etc.)...

#include "Transaction.h"
#include "PrintMacros.h"

using namespace DRAMSim;
using namespace std;

Transaction::Transaction() {}

Transaction::Transaction(TransactionType transType, uint64_t addr, void *dat)
{
	transactionType = transType;
	address = addr;
	data = dat;
}

void Transaction::print()
{
	if (transactionType == DATA_READ)
	{
		PRINT("T [Read] [0x" << hex << address << "]" << dec );
	}
	else if (transactionType == DATA_WRITE)
	{
		PRINT("T [Write] [0x" << hex << address << "] [" << dec << data << "]" );
	}
	else if (transactionType == RETURN_DATA)
	{
		PRINT("T [Data] [0x" << hex << address << "] [" << dec << data << "]" );
	}
}

