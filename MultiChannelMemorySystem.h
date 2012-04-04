#include "SimulatorObject.h"
#include "Transaction.h"
#include "SystemConfiguration.h"
#include "MemorySystem.h"


namespace DRAMSim {


class MultiChannelMemorySystem : public SimulatorObject 
{
	public: 

	MultiChannelMemorySystem(const string &dev, const string &sys, const string &pwd, const string &trc, unsigned megsOfMemory);
		virtual ~MultiChannelMemorySystem();
			bool addTransaction(Transaction &trans);
			bool addTransaction(bool isWrite, uint64_t addr);
			void update();
			void printStats();
			void RegisterCallbacks( 
				TransactionCompleteCB *readDone,
				TransactionCompleteCB *writeDone,
				void (*reportPower)(double bgpower, double burstpower, double refreshpower, double actprepower));

	void InitOutputFiles(string tracefilename);

	// mostly for other simulators
	void overrideSystemParam(string key, string value);
	void overrideSystemParam(string kvpair);

	//output file
	std::ofstream visDataOut;

	private:
		unsigned findChannelNumber(uint64_t addr);
		vector<MemorySystem*> channels; 
		unsigned megsOfMemory; 
		string deviceIniFilename;
		string systemIniFilename;
		string traceFilename;
		string pwd;
		static void mkdirIfNotExist(string path);
		static bool fileExists(string path); 

	};
}
