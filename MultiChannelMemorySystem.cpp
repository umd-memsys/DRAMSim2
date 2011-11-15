#include "MultiChannelMemorySystem.h"
#include "AddressMapping.h"
#include "IniReader.h"


using namespace DRAMSim; 


MultiChannelMemorySystem::MultiChannelMemorySystem(string deviceIniFilename, string systemIniFilename, string pwd, string trc, unsigned megsOfMemory_)
	:megsOfMemory(megsOfMemory_)
{
	if (pwd.length() > 0)
	{
		//ignore the pwd argument if the argument is an absolute path
		if (deviceIniFilename[0] != '/')
		{
			deviceIniFilename = pwd + "/" + deviceIniFilename;
		}

		if (systemIniFilename[0] != '/')
		{
			systemIniFilename = pwd + "/" + systemIniFilename;
		}
	}



	DEBUG("== Loading device model file '"<<deviceIniFilename<<"' == ");
	IniReader::ReadIniFile(deviceIniFilename, false);
	DEBUG("== Loading system model file '"<<systemIniFilename<<"' == ");
	IniReader::ReadIniFile(systemIniFilename, true);

	if (NUM_CHANS == 0) 
	{
		ERROR("Zero channels"); 
		abort(); 
	}

	printf("MM CONSTRUCTOR\n"); 
	for (size_t i=0; i<NUM_CHANS; i++)
	{
		MemorySystem *channel = new MemorySystem(i, deviceIniFilename,systemIniFilename,pwd,trc,megsOfMemory/NUM_CHANS);
		channels.push_back(channel);
	}
#ifdef LOG_OUTPUT
	char *sim_description = getenv("SIM_DESC");
	string sim_description_str;
	string dramsimLogFilename;
	dramsimLogFilename = "dramsim"; 
	if (sim_description != NULL)
	{
		sim_description_str = string(sim_description);
		dramsimLogFilename += "."+sim_description_str; 
	}
	dramsimLogFilename += ".log";

	dramsim_log.open(dramsimLogFilename.c_str(), ios_base::out | ios_base::trunc );

	if (!dramsim_log) 
	{
// ERROR("Cannot open "<< dramsimLogFilename);
	//	exit(-1); 
	}
#endif

}
MultiChannelMemorySystem::~MultiChannelMemorySystem()
{
#ifdef LOG_OUTPUT
	dramsim_log.flush();
	dramsim_log.close();
#endif
}
void MultiChannelMemorySystem::update() 
{
	for (size_t i=0; i<NUM_CHANS; i++)
	{
		channels[i]->update(); 
	}
}
unsigned MultiChannelMemorySystem::findChannelNumber(uint64_t addr)
{
	// Single channel case is a trivial shortcut case 
	if (NUM_CHANS == 1)
	{
		return 0; 
	}

	if (NUM_CHANS % 2 != 0)
	{
		ERROR("Odd number of channels not supported"); 
		abort(); 
	}

	// only chan is used from this set 
	unsigned channelNumber,rank,bank,row,col;
	addressMapping(addr, channelNumber, rank, bank, row, col); 
	if (channelNumber >= NUM_CHANS)
	{
		ERROR("Got channel index "<<channelNumber<<" but only "<<NUM_CHANS<<" exist"); 
		abort();
	}
	//DEBUG("Channel idx = "<<channelNumber<<" totalbits="<<totalBits<<" channelbits="<<channelBits); 

	return channelNumber;

}
bool MultiChannelMemorySystem::addTransaction(Transaction &trans)
{
	unsigned channelNumber = findChannelNumber(trans.address); 
	channels[channelNumber]->addTransaction(trans); 
	return true;
}

bool MultiChannelMemorySystem::addTransaction(bool isWrite, uint64_t addr)
{
	unsigned channelNumber = findChannelNumber(addr); 
	channels[channelNumber]->addTransaction(isWrite, addr); 
	return true;
}

void MultiChannelMemorySystem::printStats() {
	for (size_t i=0; i<NUM_CHANS; i++)
	{
		PRINT("==== Channel ["<<i<<"] ====");
		channels[i]->printStats(); 
		PRINT("//// Channel ["<<i<<"] ////");
	}
}
void MultiChannelMemorySystem::RegisterCallbacks( 
		TransactionCompleteCB *readDone,
		TransactionCompleteCB *writeDone,
		void (*reportPower)(double bgpower, double burstpower, double refreshpower, double actprepower))
{
	for (size_t i=0; i<NUM_CHANS; i++)
	{
		channels[i]->RegisterCallbacks(readDone, writeDone, reportPower); 
	}
}
namespace DRAMSim {
MultiChannelMemorySystem *getMultiChannelMemorySystemInstance(string dev, string sys, string pwd, string trc, unsigned megsOfMemory) 
{
	return new MultiChannelMemorySystem(dev, sys, pwd, trc, megsOfMemory);
}
}
