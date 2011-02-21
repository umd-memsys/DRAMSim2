#include "MultiChannelMemorySystem.h"
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
	if (NUM_CHANS % 2 != 0)
	{
		ERROR("Odd number of channels not supported"); 
		abort(); 
	}
	// TODO: call the address mapping here? 
	size_t channelBits = dramsim_log2(NUM_CHANS);
	size_t totalBits = dramsim_log2(megsOfMemory) + 20; 
	// all our address mapping schemes so far assume top bits for the channel
	unsigned channelNumber = (addr >> (totalBits-channelBits)) & ((1<<channelBits)-1);
	if (channelNumber >= NUM_CHANS)
	{
		ERROR("Got channel index "<<channelNumber<<" but only "<<NUM_CHANS<<" exist"); 
		abort();
	}
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
		channels[i]->printStats(); 
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
