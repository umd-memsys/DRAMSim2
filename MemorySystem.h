/*********************************************************************************
*  Copyright (c) 2010-2011, Elliott Cooper-Balis
*                             Paul Rosenfeld
*                             Bruce Jacob
*                             University of Maryland 
*                             dramninjas [at] umd [dot] edu
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

#include "SimulatorObject.h"
#include "SystemConfiguration.h"
#include "MemoryController.h"
#include "Rank.h"
#include "Transaction.h"
#include "Callback.h"
#include <deque>

namespace DRAMSim
{
class MemorySystem : public SimulatorObject
{
public:
	//functions
	MemorySystem(unsigned id, string dev, string sys, string pwd, string trc, unsigned megsOfMemory);
	virtual ~MemorySystem();
	void update();
	bool addTransaction(Transaction &trans);
	bool addTransaction(bool isWrite, uint64_t addr);
	bool addTransaction(bool isWrite, uint64_t addr, void *data, size_t dataNumBytes); 
	void printStats();
	void printStats(bool unused);
	bool WillAcceptTransaction();
	string SetOutputFileName(string tracefilename);
	void RegisterCallbacks(
	    ReadDataCB *readDone,
	    TransactionCompleteCB *writeDone,
	    void (*reportPower)(double bgpower, double burstpower, double refreshpower, double actprepower));


	// mostly for other simulators
	void overrideSystemParam(string key, string value);
	void overrideSystemParam(string kvpair);



	//fields
	// unfortunately, this is the easiest to keep C++ from initializing my members by default
	MemoryController *memoryController;
	vector<Rank> *ranks;
	deque<Transaction> pendingTransactions; 

	//output file
	std::ofstream visDataOut;

	//function pointers
	ReadDataCB *ReturnReadData;
	TransactionCompleteCB *WriteDataDone;

	//TODO: make this a functor as well?
	static powerCallBack_t ReportPower;

	unsigned systemID;

private:
	static void mkdirIfNotExist(string path);
	string deviceIniFilename;
	string systemIniFilename;
	string traceFilename;
	string pwd;
};
}

#endif

