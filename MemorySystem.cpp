/*********************************************************************************
*  Copyright (c) 2010-2011, Elliott Cooper-Balis
*                             Paul Rosenfeld
*                             Bruce Jacob
*                             University of Maryland 
*                             dramninjas [at] umd [dot] edu
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
#include "IniReader.h"
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <errno.h> 
#include <sstream> //stringstream
#include <stdlib.h> // getenv()

using namespace std;


ofstream cmd_verify_out; //used in Rank.cpp and MemoryController.cpp if VERIFICATION_OUTPUT is set

unsigned NUM_DEVICES;
unsigned NUM_RANKS;

namespace DRAMSim {
#ifdef LOG_OUTPUT
ofstream dramsim_log;
#endif

powerCallBack_t MemorySystem::ReportPower = NULL;

MemorySystem::MemorySystem(unsigned id, string deviceIniFilename, string systemIniFilename, string pwd,
                           string traceFilename, unsigned int megsOfMemory) :
		ReturnReadData(NULL),
		WriteDataDone(NULL),
		systemID(0),
		deviceIniFilename(deviceIniFilename),
		systemIniFilename(systemIniFilename),
		traceFilename(traceFilename),
		pwd(pwd)
{
	currentClockCycle = 0;
	//set ID
	systemID = id;

	DEBUG("===== MemorySystem "<<systemID<<" =====");

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

	//calculate the total storage based on the devices the user selected and the number of

	//calculate number of devices
	/************************
	  This code has always been problematic even though it's pretty simple. I'll try to explain it 
	  for my own sanity. 

	  There are two main variables here that we could let the user choose:
	  NUM_RANKS or TOTAL_STORAGE.  Since the density and width of the part is
	  fixed by the device ini file, the only variable that is really
	  controllable is the number of ranks. Users care more about choosing the
	  total amount of storage, but with a fixed device they might choose a total
	  storage that isn't possible. In that sense it's not as good to allow them
	  to choose TOTAL_STORAGE (because any NUM_RANKS value >1 will be valid).

	  However, users don't care (or know) about ranks, they care about total
	  storage, so maybe it's better to let them choose and just throw an error
	  if they choose something invalid. 

	  A bit of background: 

	  Each column contains DEVICE_WIDTH bits. A row contains NUM_COLS columns.
	  Each bank contains NUM_ROWS rows. Therefore, the total storage per DRAM device is: 
	  		PER_DEVICE_STORAGE = NUM_ROWS*NUM_COLS*DEVICE_WIDTH*NUM_BANKS (in bits)

	 A rank *must* have a 64 bit output bus (JEDEC standard), so each rank must have:
	  		NUM_DEVICES_PER_RANK = 64/DEVICE_WIDTH  
			(note: if you have multiple channels ganged together, the bus width is 
			effectively NUM_CHANS * 64/DEVICE_WIDTH)
	 
	If we multiply these two numbers to get the storage per rank (in bits), we get:
			PER_RANK_STORAGE = PER_DEVICE_STORAGE*NUM_DEVICES_PER_RANK = NUM_ROWS*NUM_COLS*NUM_BANKS*64 

	Finally, to get TOTAL_STORAGE, we need to multiply by NUM_RANKS
			TOTAL_STORAGE = PER_RANK_STORAGE*NUM_RANKS (total storage in bits)

	So one could compute this in reverse -- compute NUM_DEVICES,
	PER_DEVICE_STORAGE, and PER_RANK_STORAGE first since all these parameters
	are set by the device ini. Then, TOTAL_STORAGE/PER_RANK_STORAGE = NUM_RANKS 

	The only way this could run into problems is if TOTAL_STORAGE < PER_RANK_STORAGE,
	which could happen for very dense parts.
	*********************/

	// number of bytes per rank
	unsigned long megsOfStoragePerRank = ((((long long)NUM_ROWS * (NUM_COLS * DEVICE_WIDTH) * NUM_BANKS) * ((long long)JEDEC_DATA_BUS_BITS / DEVICE_WIDTH)) / 8) >> 20;

	// If this is set, effectively override the number of ranks
	if (megsOfMemory != 0)
	{
		NUM_RANKS = megsOfMemory / megsOfStoragePerRank;
		if (NUM_RANKS == 0)
		{
			PRINT("WARNING: Cannot create memory system with "<<megsOfMemory<<"MB, defaulting to minimum size of "<<megsOfStoragePerRank<<"MB");
			NUM_RANKS=1;
		}
	}

	NUM_DEVICES = JEDEC_DATA_BUS_BITS/DEVICE_WIDTH;
	TOTAL_STORAGE = (NUM_RANKS * megsOfStoragePerRank); 

	DEBUG("TOTAL_STORAGE : "<< TOTAL_STORAGE << "MB | "<<NUM_RANKS<<" Ranks | "<< NUM_DEVICES <<" Devices per rank");

	IniReader::InitEnumsFromStrings();
	if (!IniReader::CheckIfAllSet())
	{
		exit(-1);
	}

	memoryController = new MemoryController(this, &visDataOut);

	// TODO: change to other vector constructor?
	ranks = new vector<Rank>();

	for (size_t i=0; i<NUM_RANKS; i++)
	{
		Rank r = Rank();
		r.setId(i);
		r.attachMemoryController(memoryController);
		ranks->push_back(r);
	}


	memoryController->attachRanks(ranks);

}


void MemorySystem::overrideSystemParam(string key, string value)
{
	cerr << "Override key " <<key<<"="<<value<<endl;
	IniReader::SetKey(key, value, true);
}

void MemorySystem::overrideSystemParam(string keyValuePair)
{
	size_t equalsign=-1;
	string overrideKey, overrideVal;
	//FIXME: could use some error checks on the string
	if ((equalsign = keyValuePair.find_first_of('=')) != string::npos) {
		overrideKey = keyValuePair.substr(0,equalsign);
		overrideVal = keyValuePair.substr(equalsign+1);
		overrideSystemParam(overrideKey, overrideVal);
	}
}

MemorySystem::~MemorySystem()
{
	/* the MemorySystem should exist for all time, nothing should be destroying it */  
//	ERROR("MEMORY SYSTEM DESTRUCTOR with ID "<<systemID);
//	abort();

	delete(memoryController);
	ranks->clear();
	delete(ranks);
	if (VIS_FILE_OUTPUT) 
	{	
		visDataOut.flush();
		visDataOut.close();
	}
	if (VERIFICATION_OUTPUT)
	{
		cmd_verify_out.flush();
		cmd_verify_out.close();
	}
#ifdef LOG_OUTPUT
	dramsim_log.flush();
	dramsim_log.close();
#endif
}

bool fileExists(string path)
{
	struct stat stat_buf;
	if (stat(path.c_str(), &stat_buf) != 0) 
	{
		if (errno == ENOENT)
		{
			return false; 
		}
	}
	return true;
}

string MemorySystem::SetOutputFileName(string traceFilename)
{
	size_t lastSlash;
	string deviceName, dramsimLogFilename;
	size_t deviceIniFilenameLength = deviceIniFilename.length();
	char *sim_description = NULL;
	string sim_description_str;
	
	sim_description = getenv("SIM_DESC"); 

#ifdef LOG_OUTPUT
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
		ERROR("Cannot open "<< dramsimLogFilename);
		exit(-1); 
	}
#endif

	// create a properly named verification output file if need be
	if (VERIFICATION_OUTPUT)
	{
		string basefilename = deviceIniFilename.substr(deviceIniFilename.find_last_of("/")+1);
		string verify_filename =  "sim_out_"+basefilename;
		if (sim_description != NULL)
		{
			verify_filename += "."+sim_description_str;
		}
		verify_filename += ".tmp";
		cmd_verify_out.open(verify_filename.c_str());
		if (!cmd_verify_out)
		{
			ERROR("Cannot open "<< verify_filename);
			exit(-1);
		}
	}
	// TODO: move this to its own function or something? 
	if (VIS_FILE_OUTPUT)
	{
		// chop off the .ini if it's there
		if (deviceIniFilename.substr(deviceIniFilenameLength-4) == ".ini")
		{
			deviceName = deviceIniFilename.substr(0,deviceIniFilenameLength-4);
			deviceIniFilenameLength -= 4;
		}

		cout << deviceName << endl;

		// chop off everything past the last / (i.e. leave filename only)
		if ((lastSlash = deviceName.find_last_of("/")) != string::npos)
		{
			deviceName = deviceName.substr(lastSlash+1,deviceIniFilenameLength-lastSlash-1);
		}

		// working backwards, chop off the next piece of the directory
		if ((lastSlash = traceFilename.find_last_of("/")) != string::npos)
		{
			traceFilename = traceFilename.substr(lastSlash+1,traceFilename.length()-lastSlash-1);
		}
		if (sim_description != NULL)
		{
			traceFilename += "."+sim_description_str;
		}

		string rest;
		stringstream out,tmpNum,tmpSystemID;

		string path = "results/";
		string filename;
		if (pwd.length() > 0)
		{
			path = pwd + "/" + path;
		}

		// create the directories if they don't exist 
		mkdirIfNotExist(path);
		path = path + traceFilename + "/";
		mkdirIfNotExist(path);
		path = path + deviceName + "/";
		mkdirIfNotExist(path);

		// finally, figure out the filename
		string sched = "BtR";
		string queue = "pRank";
		if (schedulingPolicy == RankThenBankRoundRobin)
		{
			sched = "RtB";
		}
		if (queuingStructure == PerRankPerBank)
		{
			queue = "pRankpBank";
		}

		/* I really don't see how "the C++ way" is better than snprintf()  */
		out << (TOTAL_STORAGE>>10) << "GB." << NUM_CHANS << "Ch." << NUM_RANKS <<"R." <<ADDRESS_MAPPING_SCHEME<<"."<<ROW_BUFFER_POLICY<<"."<< TRANS_QUEUE_DEPTH<<"TQ."<<CMD_QUEUE_DEPTH<<"CQ."<<sched<<"."<<queue;
		if (sim_description)
		{
			out << "." << sim_description;
		}

		//filename so far, without .vis extension, see if it exists already
		filename = out.str();
		for (int i=0; i<100; i++)
		{
			if (fileExists(path+filename+tmpNum.str()+".vis"))
			{
				tmpNum.seekp(0);
				tmpNum << "." << i;
			}
			else 
			{
				filename = filename+tmpNum.str()+".vis";
				break;
			}
		}

		if (systemID!=0)
		{
			tmpSystemID<<"."<<systemID;
		}
		path.append(filename+tmpSystemID.str());

		return path;
	}
	return string("");
}

void MemorySystem::mkdirIfNotExist(string path)
{
	struct stat stat_buf;
	// dwxr-xr-x on the results directories
	if (stat(path.c_str(), &stat_buf) != 0)
	{
		if (errno == ENOENT)
		{
			DEBUG("\t directory doesn't exist, trying to create ...");
			mode_t mode = (S_IXOTH | S_IXGRP | S_IXUSR | S_IROTH | S_IRGRP | S_IRUSR | S_IWUSR) ;
			if (mkdir(path.c_str(), mode) != 0)
			{
				perror("Error Has occurred while trying to make directory: ");
				cerr << path << endl;
				abort();
			}
		}
	}
	else
	{
		if (!S_ISDIR(stat_buf.st_mode))
		{
			ERROR(path << "is not a directory");
			abort();
		}
	}
}

bool MemorySystem::WillAcceptTransaction()
{
	return true;
//	return memoryController->WillAcceptTransaction();
}
bool MemorySystem::addTransaction(bool isWrite, uint64_t addr, void *data, size_t dataNumBytes)
{
	if (!memoryController->WillAcceptTransaction())
	{
		return false; 
	}

	// Build the datapacket first
	DataPacket *dp = new DataPacket((byte*)data, dataNumBytes, addr); 

	TransactionType type = isWrite ? DATA_WRITE : DATA_READ;
	Transaction trans(type, addr, dp); 
	return addTransaction(trans); // will always be true
}
bool MemorySystem::addTransaction(bool isWrite, uint64_t addr)
{
	TransactionType type = isWrite ? DATA_WRITE : DATA_READ;
	Transaction trans(type,addr,NULL);
	// push_back in memoryController will make a copy of this during
	// addTransaction so it's kosher for the reference to be local 

	if (memoryController->WillAcceptTransaction()) 
	{
		return memoryController->addTransaction(trans); // will be true
	}
	else
	{
		pendingTransactions.push_back(trans);
		return true;
	}
}

bool MemorySystem::addTransaction(Transaction &trans)
{
	return memoryController->addTransaction(trans);
}

//prints statistics
void MemorySystem::printStats()
{
	memoryController->printStats(true);
}

void MemorySystem::printStats(bool)
{
	printStats();
}


//update the memory systems state
void MemorySystem::update()
{
	if (currentClockCycle == 0)
	{
		string visOutputFilename = SetOutputFileName(traceFilename);
		if (VIS_FILE_OUTPUT)
		{
			cerr << "writing vis file to " <<visOutputFilename<<endl;

			visDataOut.open(visOutputFilename.c_str());
			if (!visDataOut)
			{
				ERROR("Cannot open '"<<visOutputFilename<<"'");
				exit(-1);
			}
			//write out the ini config values for the visualizer tool
			IniReader::WriteValuesOut(visDataOut);
		}	
	}
	//PRINT(" ----------------- Memory System Update ------------------");

	//updates the state of each of the objects
	// NOTE - do not change order
	for (size_t i=0;i<NUM_RANKS;i++)
	{
		(*ranks)[i].update();
	}

	//pendingTransactions will only have stuff in it if MARSS is adding stuff
	if (pendingTransactions.size() > 0 && memoryController->WillAcceptTransaction())
	{
		memoryController->addTransaction(pendingTransactions.front());
		pendingTransactions.pop_front();
	}
	memoryController->update();

	//simply increments the currentClockCycle field for each object
	for (size_t i=0;i<NUM_RANKS;i++)
	{
		(*ranks)[i].step();
	}
	memoryController->step();
	this->step();

	//PRINT("\n"); // two new lines
}

void MemorySystem::RegisterCallbacks( ReadDataCB* readCB, TransactionCompleteCB* writeCB,
                                      void (*reportPower)(double bgpower, double burstpower,
                                                          double refreshpower, double actprepower))
{
	ReturnReadData = readCB;
	WriteDataDone = writeCB;
	ReportPower = reportPower;
}

// static allocator for the library interface 
MemorySystem *getMemorySystemInstance(unsigned id, string dev, string sys, string pwd, string trc, unsigned megsOfMemory)
{
	return new MemorySystem(id, dev, sys, pwd, trc, megsOfMemory);
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

