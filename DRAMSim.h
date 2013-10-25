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


#ifndef DRAMSIM_H
#define DRAMSIM_H
/*
 * This is a public header for DRAMSim including this along with libdramsim.so should
 * provide all necessary functionality to talk to an external simulator
 */
#include "Callback.h"
#include <stdio.h> 
#include <string>
#include <vector>
#include <map>
#include <list> 

using std::string;

namespace DRAMSim 
{

	typedef std::map<std::string, std::string> OptionsMap;
	typedef std::list<std::string> OptionsFailedToSet; 

	class CSVWriter; 
	class DRAMSimTransaction;
	class Config; 

	class DRAMSimInterface {
		public: 
			virtual uint64_t getCycle() = 0;
			virtual DRAMSimTransaction *makeTransaction(bool isWrite, uint64_t addr, unsigned requestSize)=0;
			virtual bool addTransaction(DRAMSimTransaction *)=0;
			virtual bool willAcceptTransaction(bool isWrite, uint64_t addr, unsigned requestSize=64, unsigned channelId=100, unsigned coreID=0) =0; 
			virtual bool addTransaction(bool isWrite, uint64_t addr, unsigned requestSize=64, unsigned channelIdx=100, unsigned coreID=0) = 0;
			virtual void update()=0;

			virtual void setCPUClockSpeed(uint64_t cpuClkFreqHz) = 0;
			virtual void simulationDone() = 0;
			virtual float getUpdateClockPeriod()=0;
			virtual void dumpStats(CSVWriter &CSVOut)=0; 

			virtual void registerCallbacks(
				TransactionCompleteCB *readDone,
				TransactionCompleteCB *writeDone,
				void (*reportPower)(double bgpower, double burstpower, double refreshpower, double actprepower)) = 0 ;
	};
//	DRAMSimInterface *getMemorySystemInstance(const string &dev, const string &sys, const string &pwd, const string &trc, unsigned megsOfMemory, CSVWriter &csvOut_, const OptionsMap *paramOverrides=NULL);
	DRAMSimInterface *getMemorySystemInstance(const std::vector<std::string> &iniFiles=std::vector<std::string>(), const string simDesc=string(""), const OptionsMap *paramOverrides=NULL); 
}

#endif
