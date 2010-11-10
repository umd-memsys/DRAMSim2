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
typedef CallbackBase<void,uint,uint64_t,uint64_t> Callback_t;
class MemorySystem : public SimulatorObject
{
public:
	//functions
	MemorySystem(uint id, string dev, string sys, string pwd, string trc);
	virtual ~MemorySystem();
	void update();
	bool addTransaction(Transaction &trans);
	bool addTransaction(bool isWrite, uint64_t addr);
	void printStats();
	void printStats(bool unused);
	bool WillAcceptTransaction();
	string SetOutputFileName(string tracefilename);
	void RegisterCallbacks(
	    Callback_t *readDone,
	    Callback_t *writeDone,
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
	Callback_t* ReturnReadData;
	Callback_t* WriteDataDone;
	//TODO: make this a functor as well?
	static powerCallBack_t ReportPower;

	uint systemID;

private:
	static void mkdirIfNotExist(string path);
	string deviceIniFilename;
	string systemIniFilename;
	string traceFilename;
	string pwd;
};
}

#endif

