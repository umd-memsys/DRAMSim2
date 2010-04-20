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

namespace DRAMSim
{
    typedef CallbackBase<void,uint,uint64_t,uint64_t> Callback_t;
	class MemorySystem : public SimulatorObject
	{
	public:
		//functions
		MemorySystem(uint id, string dev, string sys, string pwd, string trc);
		~MemorySystem();
		void update();
		bool addTransaction(Transaction &trans);
		bool addTransaction(bool isWrite, uint64_t addr);
		void printStats();
		void printStats(bool unused);
		string SetOutputFileName(string tracefilename);
	  void RegisterCallbacks( 
            Callback_t *readDone,
            Callback_t *writeDone,
            void (*reportPower)(double bgpower, double burstpower, double refreshpower, double actprepower));

	  
	  // mostly for CMPSim
		void overrideSystemParam(string key, string value);
		void overrideSystemParam(string kvpair);


	
		//fields
		// unfortunately, this is the easiest to keep C++ from initializing my members by default
		MemoryController *memoryController;
		vector<Rank> *ranks;

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

