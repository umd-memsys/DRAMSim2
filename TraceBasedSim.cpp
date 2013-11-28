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



//TraceBasedSim.cpp
//
//File to run a trace-based simulation
//

#include <iostream>
#include <fstream>
#include <sstream>
#include <getopt.h>
#include <map>
#include <deque>

#include "DRAMSim.h"
#include "SystemConfiguration.h"
#include "MemorySystem.h"
#include "MultiChannelMemorySystem.h"
#include "Transaction.h"
#include "ConfigIniReader.h"
#include "IniReader.h"
#include "CSVWriter.h"
#include <assert.h>
#include "config.h"
#ifdef HAS_GPERF 
#include <google/profiler.h>
#endif 

enum TraceType
{
	k6,
	mase,
	misc
};

using namespace DRAMSim;
using namespace std;

//#define RETURN_TRANSACTIONS 1

#ifndef _SIM_
int SHOW_SIM_OUTPUT = 1;
ofstream visDataOut; //mostly used in MemoryController

#ifdef RETURN_TRANSACTIONS
class TransactionReceiver
{
	private: 
		map<uint64_t, deque<uint64_t> > pendingReadRequests; 
		map<uint64_t, deque<uint64_t> > pendingWriteRequests; 
		unsigned numReads, numWrites; 
	
	public: 
		TransactionReceiver() : numReads(0), numWrites(0) 
		{


		}
		void add_pending(bool isWrite, uint64_t address, uint64_t cycle)
		{
			//DEBUG("Adding "<<std::hex<<address<<" on cycle "<<std::dec<<cycle<<"\n");
			// C++ deques are ordered, so the deque will always push to the back and
			// remove at the front to ensure ordering
			if (isWrite)
			{
				pendingWriteRequests[address].push_back(cycle); 
			}
			else 
			{
				pendingReadRequests[address].push_back(cycle); 
			}
		}

		void read_complete(unsigned id, uint64_t address, uint64_t done_cycle)
		{
			map<uint64_t, deque<uint64_t> >::iterator it;
			it = pendingReadRequests.find(address); 
			if (it == pendingReadRequests.end())
			{
				ERROR("Cant find a pending read for this one"); 
				exit(-1);
			}
			else
			{
				if (it->second.size() == 0)
				{
					ERROR("Nothing here, either"); 
					exit(-1); 
				}
			}

			uint64_t added_cycle = pendingReadRequests[address].front();
			uint64_t latency = done_cycle - added_cycle;

			pendingReadRequests[address].pop_front();
			numReads++;
			//cout << "Read Callback: #"<<numReads<<": 0x"<< std::hex << address << std::dec << " latency="<<latency<<"cycles ("<< done_cycle<< "->"<<added_cycle<<")"<<endl;
		}
		void write_complete(unsigned id, uint64_t address, uint64_t done_cycle)
		{
			map<uint64_t, deque<uint64_t> >::iterator it;
			it = pendingWriteRequests.find(address); 
			if (it == pendingWriteRequests.end())
			{
				ERROR("Cant find a pending read for this one"); 
				exit(-1);
			}
			else
			{
				if (it->second.size() == 0)
				{
					ERROR("Nothing here, either"); 
					exit(-1); 
				}
			}

			uint64_t added_cycle = pendingWriteRequests[address].front();
			uint64_t latency = done_cycle - added_cycle;

			pendingWriteRequests[address].pop_front();
			numWrites++;
			//cout << "Write Callback: #"<<numWrites<<" 0x"<< std::hex << address << std::dec << " latency="<<latency<<"cycles ("<< done_cycle<< "->"<<added_cycle<<")"<<endl;
		}
		void simulationDone(uint64_t cycles) {
			DEBUG("Transaction receiver got back "<<numReads<<" reads and "<<numWrites<<" writes in "<<cycles<<" cycles\n");
		}
};

TransactionReceiver transactionReceiver; 

#endif

void usage()
{
	cout << "DRAMSim2 Usage: " << endl;
	cout << "DRAMSim -t tracefile -s system.ini -d ini/device.ini [-c #] [-p pwd] [-q] [-S 2048] [-n] [-o OPTION_A=1234,tRC=14,tFAW=19]" <<endl;
	cout << "\t-t, --tracefile=FILENAME \tspecify a tracefile to run  "<<endl;
	cout << "\t-s, --systemini=FILENAME \tspecify an ini file that describes the memory system parameters  "<<endl;
	cout << "\t-d, --deviceini=FILENAME \tspecify an ini file that describes the device-level parameters"<<endl;
	cout << "\t-c, --numcycles=# \t\tspecify number of cycles to run the simulation for [default=30] "<<endl;
	cout << "\t-q, --quiet \t\t\tflag to suppress simulation output (except final stats) [default=no]"<<endl;
	cout << "\t-o, --option=OPTION_A=234,tFAW=14\t\t\toverwrite any ini file option from the command line"<<endl;
	cout << "\t-p, --pwd=DIRECTORY\t\tSet the working directory (i.e. usually DRAMSim directory where ini/ and results/ are)"<<endl;
	cout << "\t-S, --size=# \t\t\tSize of the memory system in megabytes [default=2048M]"<<endl;
	cout << "\t-n, --notiming \t\t\tDo not use the clock cycle information in the trace file"<<endl;
	cout << "\t-v, --visfile \t\t\tVis output filename"<<endl;
#if HAS_GPERF
	cout << "\t-P, --profiler \t\t\tEnable gperf profiling"<<endl;
#endif
}
#endif

void *parseTraceFileLine(const string &line, uint64_t &addr, enum TransactionType &transType, uint64_t &clockCycle, TraceType type, bool useClockCycle)
{
	size_t previousIndex=0;
	size_t spaceIndex=0;
	uint64_t *dataBuffer = NULL;
	string addressStr="", cmdStr="", dataStr="", ccStr="";

	switch (type)
	{
	case k6:
	{
		spaceIndex = line.find_first_of(" ", 0);

		addressStr = line.substr(0, spaceIndex);
		previousIndex = spaceIndex;

		spaceIndex = line.find_first_not_of(" ", previousIndex);
		cmdStr = line.substr(spaceIndex, line.find_first_of(" ", spaceIndex) - spaceIndex);
		previousIndex = line.find_first_of(" ", spaceIndex);

		spaceIndex = line.find_first_not_of(" ", previousIndex);
		ccStr = line.substr(spaceIndex, line.find_first_of(" ", spaceIndex) - spaceIndex);

		if (cmdStr.compare("P_MEM_WR")==0 ||
		        cmdStr.compare("BOFF")==0)
		{
			transType = DATA_WRITE;
		}
		else if (cmdStr.compare("P_FETCH")==0 ||
		         cmdStr.compare("P_MEM_RD")==0 ||
		         cmdStr.compare("P_LOCK_RD")==0 ||
		         cmdStr.compare("P_LOCK_WR")==0)
		{
			transType = DATA_READ;
		}
		else
		{
			ERROR("== Unknown Command : "<<cmdStr);
			exit(0);
		}

		istringstream a(addressStr.substr(2));//gets rid of 0x
		a>>hex>>addr;

		//if this is set to false, clockCycle will remain at 0, and every line read from the trace
		//  will be allowed to be issued
		if (useClockCycle)
		{
			istringstream b(ccStr);
			b>>clockCycle;
		}
		break;
	}
	case mase:
	{
		spaceIndex = line.find_first_of(" ", 0);

		addressStr = line.substr(0, spaceIndex);
		previousIndex = spaceIndex;

		spaceIndex = line.find_first_not_of(" ", previousIndex);
		cmdStr = line.substr(spaceIndex, line.find_first_of(" ", spaceIndex) - spaceIndex);
		previousIndex = line.find_first_of(" ", spaceIndex);

		spaceIndex = line.find_first_not_of(" ", previousIndex);
		ccStr = line.substr(spaceIndex, line.find_first_of(" ", spaceIndex) - spaceIndex);

		if (cmdStr.compare("IFETCH")==0||
		        cmdStr.compare("READ")==0)
		{
			transType = DATA_READ;
		}
		else if (cmdStr.compare("WRITE")==0)
		{
			transType = DATA_WRITE;
		}
		else
		{
			ERROR("== Unknown command in tracefile : "<<cmdStr);
		}

		istringstream a(addressStr.substr(2));//gets rid of 0x
		a>>hex>>addr;

		//if this is set to false, clockCycle will remain at 0, and every line read from the trace
		//  will be allowed to be issued
		if (useClockCycle)
		{
			istringstream b(ccStr);
			b>>clockCycle;
		}

		break;
	}
	case misc:
		spaceIndex = line.find_first_of(" ", spaceIndex+1);
		if (spaceIndex == string::npos)
		{
			ERROR("Malformed line: '"<< line <<"'");
		}

		addressStr = line.substr(previousIndex,spaceIndex);
		previousIndex=spaceIndex;

		spaceIndex = line.find_first_of(" ", spaceIndex+1);
		if (spaceIndex == string::npos)
		{
			cmdStr = line.substr(previousIndex+1);
		}
		else
		{
			cmdStr = line.substr(previousIndex+1,spaceIndex-previousIndex-1);
			dataStr = line.substr(spaceIndex+1);
		}

		//convert address string -> number
		istringstream b(addressStr.substr(2)); //substr(2) chops off 0x characters
		b >>hex>> addr;

		// parse command
		if (cmdStr.compare("read") == 0)
		{
			transType=DATA_READ;
		}
		else if (cmdStr.compare("write") == 0)
		{
			transType=DATA_WRITE;
		}
		else
		{
			ERROR("INVALID COMMAND '"<<cmdStr<<"'");
			exit(-1);
		}
		if (SHOW_SIM_OUTPUT)
		{
			DEBUGN("ADDR='"<<hex<<addr<<dec<<"',CMD='"<<transType<<"'");//',DATA='"<<dataBuffer[0]<<"'");
		}

		//parse data
		//if we are running in a no storage mode, don't allocate space, just return NULL
#ifndef NO_STORAGE
		if (dataStr.size() > 0 && transType == DATA_WRITE)
		{
			// 32 bytes of data per transaction
			dataBuffer = (uint64_t *)calloc(sizeof(uint64_t),4);
			size_t strlen = dataStr.size();
			for (int i=0; i < 4; i++)
			{
				size_t startIndex = i*16;
				if (startIndex > strlen)
				{
					break;
				}
				size_t charsLeft = min(((size_t)16), strlen - startIndex + 1);
				string piece = dataStr.substr(i*16,charsLeft);
				istringstream iss(piece);
				iss >> hex >> dataBuffer[i];
			}
			PRINTN("\tDATA=");
			BusPacket::printData(dataBuffer);
		}

		PRINT("");
#endif
		break;
	}
	return dataBuffer;
}

#ifndef _SIM_

/** 
 * Override options can be specified on the command line as -o key1=value1,key2=value2
 * this method should parse the key-value pairs and put them into a map 
 **/ 
OptionsMap parseParamOverrides(const string &kv_str)
{
	OptionsMap kv_map;
	size_t start = 0, comma=0, equal_sign=0;
	// split the commas if they are there
	while (1)
	{
		equal_sign = kv_str.find('=', start); 
		if (equal_sign == string::npos)
		{
			break;
		}

		comma = kv_str.find(',', equal_sign);
		if (comma == string::npos)
		{
			comma = kv_str.length();
		}

		string key = kv_str.substr(start, equal_sign-start);
		string value = kv_str.substr(equal_sign+1, comma-equal_sign-1); 

		kv_map[key] = value; 
		start = comma+1;

	}
	return kv_map; 
}
bool parseLineAndTryAdd(const string &line, TraceType traceType, DRAMSimInterface *memorySystem, uint64_t currentClockCycle, bool useClockCycle) {
	DRAMSimTransaction *trans=NULL;
	uint64_t addr=0;
	uint64_t clockCycle=0;
	enum TransactionType transType;

	//DEBUG("LINE='"<<line<<"'\n");
	parseTraceFileLine(line, addr, transType,clockCycle, traceType,useClockCycle);
	bool isWrite = (transType == DATA_WRITE);
	trans = memorySystem->makeTransaction(isWrite, addr, 64); 

	if (trans && currentClockCycle >= clockCycle)
	{
		bool accepted = memorySystem->addTransaction(trans);
		assert(accepted);
#ifdef RETURN_TRANSACTIONS
		transactionReceiver.add_pending(isWrite, addr, currentClockCycle); 
#endif
		return true; 
	}
	return false;
}

int main(int argc, char **argv)
{
	std::cerr << "********************************************************************************\n";
	std::cerr << "* DRAMSim2: A Cycle-Accurate DRAM Simulator built from:" stringify(GIT_VERSION)<<"\n";
	std::cerr << "                                                                               *\n";
#ifdef GIT_VERSION
	std::cerr << "* 	built from: " stringify(GIT_VERSION)<<"\n";
#endif
	std::cerr << "*                                                                               \n";
	std::cerr << "********************************************************************************\n";

	int c;
	TraceType traceType;
	string traceFileName;
	string systemIniFilename("system.ini");
	string deviceIniFilename;
	string pwdString;
	unsigned megsOfMemory=2048;
	bool useClockCycle=true;
#ifdef HAS_GPERF
	bool enableCPUprofiler=false; 
#endif
	
	OptionsMap paramOverrides; 

	unsigned numCycles=1000;
	//getopt stuff
	while (1)
	{
		static struct option long_options[] =
		{
			{"deviceini", required_argument, 0, 'd'},
			{"tracefile", required_argument, 0, 't'},
			{"systemini", required_argument, 0, 's'},

			{"pwd", required_argument, 0, 'p'},
			{"numcycles",  required_argument,	0, 'c'},
			{"option",  required_argument,	0, 'o'},
			{"quiet",  no_argument, &SHOW_SIM_OUTPUT, 'q'},
			{"help", no_argument, 0, 'h'},
			{"size", required_argument, 0, 'S'},
			{"visfile", required_argument, 0, 'v'},
			{0, 0, 0, 0}
		};
		int option_index=0; //for getopt
		c = getopt_long (argc, argv, "t:s:c:d:o:p:S:qnP", long_options, &option_index);
		if (c == -1)
		{
			break;
		}
		switch (c)
		{
		case 0: //TODO: figure out what the hell this does, cuz it never seems to get called
			if (long_options[option_index].flag != 0) //do nothing on a flag
			{
				printf("setting flag\n");
				break;
			}
			printf("option %s",long_options[option_index].name);
			if (optarg)
			{
				printf(" with arg %s", optarg);
			}
			printf("\n");
			break;
		case 'h':
			usage();
			exit(0);
			break;
		case 't':
			traceFileName = string(optarg);
			break;
		case 's':
			systemIniFilename = string(optarg);
			break;
		case 'd':
			deviceIniFilename = string(optarg);
			break;
		case 'c':
			numCycles = atoi(optarg);
			break;
		case 'S':
			megsOfMemory=atoi(optarg);
			break;
		case 'p':
			pwdString = string(optarg);
			break;
		case 'q':
			SHOW_SIM_OUTPUT=false;
			break;
		case 'n':
			useClockCycle=false;
			break;
		case 'o':
			paramOverrides = parseParamOverrides(string(optarg)); 
			break;
		case 'P':
#ifdef HAS_GPERF
			std::cerr << "Enabling CPU Profiler\n";
			enableCPUprofiler=true;
#else
			std::cerr << "No gperf support detected, ignoring flag\n"; 
#endif
			break;
		case '?':
			usage();
			exit(-1);
			break;
		}
	}

	// get the trace filename
	string temp = traceFileName.substr(traceFileName.find_last_of("/")+1);

	//get the prefix of the trace name
	temp = temp.substr(0,temp.find_first_of("_"));
	if (temp=="mase")
	{
		traceType = mase;
	}
	else if (temp=="k6")
	{
		traceType = k6;
	}
	else if (temp=="misc")
	{
		traceType = misc;
	}
	else
	{
		ERROR("== Unknown Tracefile Type : "<<temp);
		exit(0);
	}


	// no default value for the default model name
	if (deviceIniFilename.length() == 0)
	{
		ERROR("Please provide a device ini file");
		usage();
		exit(-1);
	}


	//ignore the pwd argument if the argument is an absolute path
	if (pwdString.length() > 0 && traceFileName[0] != '/')
		traceFileName = pwdString + "/" +traceFileName;
	if (pwdString.length() > 0 && systemIniFilename[0] != '/')
		systemIniFilename = pwdString + "/" + systemIniFilename;
	if (pwdString.length() > 0 && deviceIniFilename[0] != '/')
		deviceIniFilename = pwdString + "/" + deviceIniFilename; 

	vector<std::string> iniFiles;
	iniFiles.push_back(deviceIniFilename); 
	iniFiles.push_back(systemIniFilename);

	ostringstream oss; 
	oss << megsOfMemory; 

	paramOverrides["megsOfMemory"] = oss.str(); 
	DRAMSimInterface *memorySystem = getMemorySystemInstance(iniFiles, "", &paramOverrides);


	// set the frequency ratio to 1:1
	memorySystem->setCPUClockSpeed(0); 

	DEBUG("== Loading trace file '"<<traceFileName<<"' == ");

	ifstream traceFile;
	string line;



#ifdef RETURN_TRANSACTIONS
	/* create and register our callback functions */
	TransactionCompleteCB *read_cb = new Callback<TransactionReceiver, void, unsigned, uint64_t, uint64_t>(&transactionReceiver, &TransactionReceiver::read_complete);
	TransactionCompleteCB *write_cb = new Callback<TransactionReceiver, void, unsigned, uint64_t, uint64_t>(&transactionReceiver, &TransactionReceiver::write_complete);
	memorySystem->registerCallbacks(read_cb, write_cb, NULL);
#endif


	int lineNumber = 0;
	bool lastTransactionSucceeded=true; 

#if HAS_GPERF
	if (enableCPUprofiler) {
		static const char *gperf_filename = "DRAMSim.gperf.prof";
		ProfilerStart(gperf_filename);
	}
#endif

	traceFile.open(traceFileName.c_str());

	if (!traceFile.is_open())
	{
		cout << "== Error - Could not open trace file"<<endl;
		exit(0);
	}
	for (size_t i=0;i<numCycles;i++)
	{
		// if the last transaction failed, try to re-add the previous line
		if (!lastTransactionSucceeded) {
			lastTransactionSucceeded = parseLineAndTryAdd(line, traceType,  memorySystem, i, useClockCycle);
		}
		// otherwise, grab the next line from the trace and try to send it
		else {
			if (!traceFile.eof())
			{
				// skip any blank lines in the trace 
				while (true) {
					getline(traceFile, line);
					lineNumber++;
					if (line.length() == 0) {
						DEBUG("Skipping blank line "<<lineNumber<<"\n");
					} else {
						break;
					}
				}
				lastTransactionSucceeded = parseLineAndTryAdd(line, traceType, memorySystem, i, useClockCycle);
			}
		}
		memorySystem->update();
	} // end main loop 

	traceFile.close();
	memorySystem->simulationDone();
#ifdef HAS_GPERF
	if (enableCPUprofiler) {
		ProfilerStop();
	}
#endif 
#ifdef RETURN_TRANSACTIONS
	transactionReceiver.simulationDone(numCycles);
#endif
}
#endif
