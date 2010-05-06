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






#ifndef TRANSACTION_H
#define TRANSACTION_H

//Transaction.h
//
//Header file for transaction object

#include "SystemConfiguration.h"

using namespace std;

namespace DRAMSim
{
enum TransactionType
{
	DATA_READ,
	DATA_WRITE,
	RETURN_DATA
};

class Transaction
{
public:
	//fields
	TransactionType transactionType;
	uint64_t address;
	void *data;
	uint64_t timeAdded;
	uint64_t timeReturned;


	//functions
	Transaction(TransactionType transType, uint64_t addr, void *data);
	Transaction();

	void print();
};
}

#endif

