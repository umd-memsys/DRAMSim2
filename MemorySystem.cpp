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




//MemorySystem.cpp
//
//Class file for JEDEC memory system wrapper
//

#include "MemorySystem.h"
#include <unistd.h>

#include "MemoryController.h"
#include "Rank.h"
#include "Transaction.h"
#include "ConfigIniReader.h"

using namespace std;


ofstream cmd_verify_out; //used in Rank.cpp and MemoryController.cpp if VERIFICATION_OUTPUT is set

namespace DRAMSim {

powerCallBack_t MemorySystem::ReportPower = NULL;

MemorySystem::MemorySystem(unsigned id, unsigned int megsOfMemory, const Config &cfg_, ostream &dramsim_log_) :
		cfg(cfg_),
		dramsim_log(dramsim_log_),
		ReturnReadData(NULL),
		WriteDataDone(NULL),
		systemID(id)
{
	DEBUG("===== MemorySystem "<<systemID<<" =====");
	DEBUG("CH. " <<systemID<<" TOTAL_STORAGE : "<< cfg.megsOfMemory << "MB | "<<cfg.NUM_RANKS<<" Ranks | "<< cfg.NUM_DEVICES <<" Devices per rank");

	// FIXME: can also just make this straight inside the class instead of as a pointer
	memoryController = new MemoryController(this, dramsim_log);

	// FIXME: no reason to have ranks be a vector<...> *, just put it straight in  
	ranks = new vector<Rank *>();

	for (size_t i=0; i<cfg.NUM_RANKS; i++)
	{
		Rank *r = new Rank(*memoryController, dramsim_log);
		r->setId(i);
		ranks->push_back(r);
	}

	memoryController->attachRanks(ranks);

}



MemorySystem::~MemorySystem()
{
	/* the MemorySystem should exist for all time, nothing should be destroying it */  
//	ERROR("MEMORY SYSTEM DESTRUCTOR with ID "<<systemID);
//	abort();

	delete(memoryController);
	memoryController=NULL; 

	for (size_t i=0; i<cfg.NUM_RANKS; i++)
	{
		delete (*ranks)[i];
	}
	ranks->clear();
	delete(ranks);

	if (cfg.VERIFICATION_OUTPUT)
	{
		cmd_verify_out.flush();
		cmd_verify_out.close();
	}
}

bool MemorySystem::WillAcceptTransaction()
{
	return memoryController->WillAcceptTransaction();
}

bool MemorySystem::addTransaction(bool isWrite, uint64_t addr)
{
	TransactionType type = isWrite ? DATA_WRITE : DATA_READ;
	Transaction *trans = new Transaction(type,addr,NULL);

	if (memoryController->WillAcceptTransaction()) 
	{
		return memoryController->addTransaction(trans);
	}
	else
	{
		pendingTransactions.push_back(trans);
		return true;
	}
}

bool MemorySystem::addTransaction(Transaction *trans)
{
	return memoryController->addTransaction(trans);
}

//prints statistics
void MemorySystem::printStats(CSVWriter *CSVOut, bool finalStats)
{
	memoryController->printStats(CSVOut, finalStats);
}


//update the memory systems state
void MemorySystem::update()
{

	//PRINT(" ----------------- Memory System Update ------------------");

	//updates the state of each of the objects
	// NOTE - do not change order
	for (size_t i=0;i<cfg.NUM_RANKS;i++)
	{
		(*ranks)[i]->update();
	}

	//pendingTransactions will only have stuff in it if MARSS is adding stuff
	if (pendingTransactions.size() > 0 && memoryController->WillAcceptTransaction())
	{
		memoryController->addTransaction(pendingTransactions.front());
		pendingTransactions.pop_front();
	}
	memoryController->update();

	//simply increments the currentClockCycle field for each object
	for (size_t i=0;i<cfg.NUM_RANKS;i++)
	{
		(*ranks)[i]->step();
	}
	memoryController->step();
	this->step();

}

void MemorySystem::RegisterCallbacks( Callback_t* readCB, Callback_t* writeCB,
                                      void (*reportPower)(double bgpower, double burstpower,
                                                          double refreshpower, double actprepower))
{
	ReturnReadData = readCB;
	WriteDataDone = writeCB;
	ReportPower = reportPower;
}

} /*namespace DRAMSim */



// This function can be used by autoconf AC_CHECK_LIB since
// apparently it can't detect C++ functions.
// Basically just an entry in the symbol table
extern "C"
{
	void libdramsim_is_present(void)
	{
		;
	}
}

