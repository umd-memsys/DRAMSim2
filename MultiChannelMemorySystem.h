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

#include <vector>
#include "SimulatorObject.h"
#include "DRAMSim.h"
#include "SystemConfiguration.h"
#include "ConfigIniReader.h"
#include "ClockDomain.h"
#include "Util.h"


using std::vector;
namespace DRAMSim {

class MemorySystem;
class Transaction;
class CSVWriter; 

class MultiChannelMemorySystem : public DRAMSimInterface, public SimulatorObject
{
	public: 

	MultiChannelMemorySystem(const Config &cfg_, ostream &logFile_=*(new onullstream())); 

		virtual ~MultiChannelMemorySystem();
		uint64_t getCycle() { return currentClockCycle; }
			bool addTransaction(Transaction *trans);
			bool addTransaction(const Transaction &trans);
			bool addTransaction(bool isWrite, uint64_t addr, unsigned requestSize=64, unsigned channelIdx=100, unsigned coreID=0);
			bool willAcceptTransaction(bool isWrite, uint64_t addr, unsigned requestSize=64, unsigned channelIdx=100, unsigned coreID=0); 
			bool willAcceptTransaction(); 
			void update();
			void printStats(bool finalStats=false);
			ostream *getLogFile();
			void simulationDone();
			float getUpdateClockPeriod() {
				return cfg.tCK;//*1E-9f;
			}
			void registerCallbacks( 
				TransactionCompleteCB *readDone,
				TransactionCompleteCB *writeDone,
				void (*reportPower)(double bgpower, double burstpower, double refreshpower, double actprepower));

	void setCPUClockSpeed(uint64_t cpuClkFreqHz);
	void dumpStats(CSVWriter &CSVOut) {
		printStats(false); 
	}

	//output file
	


	private:
		const Config &cfg; 

		unsigned findChannelNumber(uint64_t addr);
		void actual_update(); 
		vector<MemorySystem*> channels; 
		ClockDomain::ClockDomainCrosser clockDomainCrosser; 
		CSVWriter *CSVOut; 
		unsigned dumpInterval;
		std::ostream &dramsim_log; 
	public: 

	void enableStatsDump(CSVWriter *CSVOut_, unsigned dumpInterval_=0) {
		CSVOut = CSVOut_;
		dumpInterval=dumpInterval_;
	}

};

}
