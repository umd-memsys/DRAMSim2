/*********************************************************************************
*  Copyright (c) 2010-2011, Elliott Cooper-Balis
*                             Paul Rosenfeld
*                             Bruce Jacob
*                             University of Maryland 
*                             dramninjas [at] gmail [dot] com
*  All rights reserved.
*  
*  Redistribution and use in source and binary forms, with or without
*  modification, are permitted provided that the following conditions are met:
*  
*     * Redistributions of source code must retain the above copyright notice,
*        this list of conditions and the following disclaimer.
*  
*     * Redistributions in binary form must reproduce the above copyright notice,
*        this list of conditions and the following disclaimer in the documentation
*        and/or other materials provided with the distribution.
*  
*  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
*  ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
*  WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
*  DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
*  FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
*  DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
*  SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
*  CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
*  OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
*  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*********************************************************************************/



#ifndef MEMORYSYSTEM_H
#define MEMORYSYSTEM_H

//MemorySystem.h
//
//Header file for JEDEC memory system wrapper
//
#include <iostream>
#include <deque>
#include <vector>
#include "Callback.h"
#include "SimulatorObject.h"
#include "MemoryController.h"

using std::ostream; 
using std::deque; 
using std::vector; 

namespace DRAMSim
{
class Config; 
class CSVWriter; 
class Rank; 
class Transaction; 
class AddressMapper;


class MemorySystem : public SimulatorObject
{
public:
	//functions
	MemorySystem(unsigned id, unsigned megsOfMemory, const Config &cfg_, AddressMapper &addressMapper, ostream &dramsim_log_);
	virtual ~MemorySystem();
	void update();
	bool addTransaction(Transaction *trans);
	bool addTransaction(bool isWrite, uint64_t addr);
	void printStats(CSVWriter *CSVOut, bool finalStats);
	bool WillAcceptTransaction();
	void registerCallbacks( TransactionCompleteCB *readDone, TransactionCompleteCB *writeDone, PowerCallback_t *powerCB);

	//fields
	const Config &cfg; 
	ostream &dramsim_log;
	AddressMapper &addressMapper;
	MemoryController memoryController;
	vector<Rank *> *ranks;
	deque<Transaction *> pendingTransactions; 
	unsigned systemID;
};
}

#endif

