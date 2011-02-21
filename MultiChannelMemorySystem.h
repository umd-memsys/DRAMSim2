#include "SimulatorObject.h"
#include "Transaction.h"
#include "SystemConfiguration.h"
#include "MemorySystem.h"


namespace DRAMSim {


class MultiChannelMemorySystem : public SimulatorObject 
{
	public: 

	MultiChannelMemorySystem(string dev, string sys, string pwd, string trc, unsigned megsOfMemory);
			bool addTransaction(Transaction &trans);
			bool addTransaction(bool isWrite, uint64_t addr);
			void update();
			void printStats();
			void RegisterCallbacks( 
				TransactionCompleteCB *readDone,
				TransactionCompleteCB *writeDone,
				void (*reportPower)(double bgpower, double burstpower, double refreshpower, double actprepower));
	private:
		unsigned findChannelNumber(uint64_t addr);
		vector<MemorySystem*> channels; 
		unsigned megsOfMemory; 

	};
	MultiChannelMemorySystem *getMultiChannelMemorySystemInstance(string dev, string sys, string pwd, string trc, unsigned megsOfMemory);
}
