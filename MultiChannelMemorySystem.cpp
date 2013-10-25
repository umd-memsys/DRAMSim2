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
#include "MultiChannelMemorySystem.h"
#include "AddressMapping.h"
#include "MemorySystem.h"
#include "Transaction.h"
#include "IniReader.h"
#include "CSVWriter.h"
#include "Util.h"



using namespace DRAMSim;


/**
 * Constructor. 
 * @param cfg a Config object that should be allocated on the heap. DRAMSim2 will take ownership of this object and be responsible for freeing it. For safety, once passed to this constructor, a config struct will never be changed.
 */ 
MultiChannelMemorySystem::MultiChannelMemorySystem(const Config &cfg_, ostream &logFile_)
	: cfg(cfg_) 
	, clockDomainCrosser(new ClockDomain::Callback<MultiChannelMemorySystem, void>(this, &MultiChannelMemorySystem::actual_update))
	, CSVOut(NULL)
	, dumpInterval(0)
	, dramsim_log(logFile_) 
{

	// A few sanity checks before we begin 
	if (!isPowerOfTwo(cfg.megsOfMemory))
	{
		ERROR("Please specify a power of 2 memory size"); 
		abort(); 
	}

	if (cfg.NUM_CHANS == 0) 
	{
		ERROR("Zero channels"); 
		abort(); 
	}

	for (size_t i=0; i<cfg.NUM_CHANS; i++)
	{
		MemorySystem *channel = new MemorySystem(i, cfg.megsOfMemory/cfg.NUM_CHANS, cfg, dramsim_log);
		channels.push_back(channel);
	}
}

/**
 * Initialize the ClockDomainCrosser to use the CPU speed If cpuClkFreqHz == 0,
 * then assume a 1:1 ratio (like for TraceBasedSim) 
 **/
void MultiChannelMemorySystem::setCPUClockSpeed(uint64_t cpuClkFreqHz)
{
	uint64_t dramsimClkFreqHz = static_cast<uint64_t>(1.0/(cfg.tCK*1e-9));
	clockDomainCrosser.clock1 = dramsimClkFreqHz; 
	clockDomainCrosser.clock2 = (cpuClkFreqHz == 0) ? dramsimClkFreqHz : cpuClkFreqHz; 
}

MultiChannelMemorySystem::~MultiChannelMemorySystem()
{
	for (size_t i=0; i<cfg.NUM_CHANS; i++)
	{
		delete channels[i];
	}
	channels.clear(); 

	dramsim_log.flush();

	/*
	if (VIS_FILE_OUTPUT) 
	{	
		visDataOut.flush();
		visDataOut.close();
	}
	*/
}
void MultiChannelMemorySystem::update()
{
	clockDomainCrosser.update(); 
}
void MultiChannelMemorySystem::actual_update() 
{
	if (currentClockCycle == 0)
	{
		DEBUG("DRAMSim2 Clock Frequency ="<<clockDomainCrosser.clock1<<"Hz, CPU Clock Frequency="<<clockDomainCrosser.clock2<<"Hz"); 
	}

	if (dumpInterval > 0 && currentClockCycle % dumpInterval == 0)
	{
		printStats(false); 
	}
	
	for (size_t i=0; i<cfg.NUM_CHANS; i++)
	{
		channels[i]->update(); 
	}

	currentClockCycle++; 
}
unsigned MultiChannelMemorySystem::findChannelNumber(uint64_t addr)
{
	// Single channel case is a trivial shortcut case 
	if (cfg.NUM_CHANS == 1)
	{
		return 0; 
	}

	if (!isPowerOfTwo(cfg.NUM_CHANS))
	{
		ERROR("We can only support power of two # of channels.\n" <<
				"I don't know what Intel was thinking, but trying to address map half a bit is a neat trick that we're not sure how to do"); 
		abort(); 
	}

	// only chan is used from this set 
	unsigned channelNumber,rank,bank,row,col;
	addressMapping(addr, channelNumber, rank, bank, row, col,cfg); 
	if (channelNumber >= cfg.NUM_CHANS)
	{
		ERROR("Got channel index "<<channelNumber<<" but only "<<cfg.NUM_CHANS<<" exist"); 
		abort();
	}
	//DEBUG("Channel idx = "<<channelNumber<<" totalbits="<<totalBits<<" channelbits="<<channelBits); 

	return channelNumber;
}

bool MultiChannelMemorySystem::addTransaction(const Transaction &trans)
{
	// copy the transaction and send the pointer to the new transaction 
	return addTransaction(new Transaction(trans)); 
}

bool MultiChannelMemorySystem::addTransaction(Transaction *trans)
{
	unsigned channelNumber = findChannelNumber(trans->address); 
	return channels[channelNumber]->addTransaction(trans); 
}

bool MultiChannelMemorySystem::addTransaction(bool isWrite, uint64_t addr, unsigned, unsigned, unsigned)
{
	unsigned channelNumber = findChannelNumber(addr); 
	return channels[channelNumber]->addTransaction(isWrite, addr); 
}

/*
	This function has two flavors: one with and without the address. 
	If the simulator won't give us an address and we have multiple channels, 
	we have to assume the worst and return false if any channel won't accept. 

	However, if the address is given, we can just map the channel and check just
	that memory controller
*/

bool MultiChannelMemorySystem::willAcceptTransaction(bool isWrite, uint64_t addr, unsigned, unsigned, unsigned)
{
	unsigned chan, rank,bank,row,col; 
	addressMapping(addr, chan, rank, bank, row, col,cfg); 
	return channels[chan]->WillAcceptTransaction(); 
}

bool MultiChannelMemorySystem::willAcceptTransaction()
{
	for (size_t c=0; c<cfg.NUM_CHANS; c++) {
		if (!channels[c]->WillAcceptTransaction())
		{
			return false; 
		}
	}
	return true; 
}



void MultiChannelMemorySystem::printStats(bool finalStats) {

	if (!CSVOut) {
		DEBUG("WARNING: printStats called even though no CSVWriter was given");
		return; 
	}

	(*CSVOut) << "ms" <<currentClockCycle * cfg.tCK * 1E-6; 
	for (size_t i=0; i<cfg.NUM_CHANS; i++)
	{
		PRINT("==== Channel ["<<i<<"] ====");
		channels[i]->printStats(CSVOut, finalStats); 
		PRINT("//// Channel ["<<i<<"] ////");
	}
}

void MultiChannelMemorySystem::registerCallbacks( 
		TransactionCompleteCB *readDone,
		TransactionCompleteCB *writeDone,
		void (*reportPower)(double bgpower, double burstpower, double refreshpower, double actprepower))
{
	for (size_t i=0; i<cfg.NUM_CHANS; i++)
	{
		channels[i]->RegisterCallbacks(readDone, writeDone, reportPower); 
	}
}
void MultiChannelMemorySystem::simulationDone() {
	printStats(true); 
}

namespace DRAMSim {
	/**
	 * Get a default DRAMSimInterface instance. The instance parameters will be set from the list of iniFiles and from the options map. The output file names will be DRAMSim.[simDesc].{csv,log}. 
	 * @param iniFiles A list of ini file names to load. Note, the way the iniReader works, files later in the list will override files earlier in the list 
	 * @param simDesc A description that will be appended to output files (.log, .csv, etc) 
	 * @param paramOverride An list of any options to manually override (will be applied after loading ini files)
	 */
	DRAMSimInterface *getMemorySystemInstance(const vector<std::string> &iniFiles, const string simDesc, const OptionsMap *paramOverrides) 
	{
		string baseName = ""; 
		if (simDesc.length() > 0 ) {
			baseName="."+simDesc; 
		}

		// setup the filenames for output files 
		// TODO: add number suffixes to avoid overwriting old ones? 
		const string visFilenamePrefix("DRAMSim"); 
		const string visFilenameSuffix(".csv");
		const string logFilenamePrefix("DRAMSim");
		const string logFilenameSuffix(".log");
		string visFilename = visFilenamePrefix + baseName + visFilenameSuffix; 
		string logFilename = logFilenamePrefix + baseName + logFilenameSuffix; 
		
		CSVWriter &CSVOut = CSVWriter::GetCSVWriterInstance(visFilename); 
		std::ostream &logFile = *(new std::ofstream(logFilename.c_str()));
		
		Config &cfg = (*new Config()); 
		for (size_t i=0; i < iniFiles.size(); i++) {
			cfg.set(IniReader::ReadIniFile(iniFiles[i]));
		}
		if (paramOverrides) {
			cfg.set(*paramOverrides);
		}

		cfg.finalize();

		MultiChannelMemorySystem *memorySystem = new MultiChannelMemorySystem(cfg, logFile);
		// tell the memory system to do its own dumping of stats (no explicit calls to dumpStats() are required)
		memorySystem->enableStatsDump(&CSVOut, cfg.EPOCH_LENGTH);
		return memorySystem;
	}
}
