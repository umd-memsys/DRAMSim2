/****************************************************************************
*	 DRAMSim2: A Cycle Accurate DRAM simulator 
*	 
*	 Copyright (C) 2010   	Elliott Cooper-Balis
*									Paul Rosenfeld 
*									Bruce Jacob
*									University of Maryland
*
*	 This program is free software: you can redistribute it and/or modify
*	 it under the terms of the GNU General Public License as published by
*	 the Free Software Foundation, either version 3 of the License, or
*	 (at your option) any later version.
*
*	 This program is distributed in the hope that it will be useful,
*	 but WITHOUT ANY WARRANTY; without even the implied warranty of
*	 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*	 GNU General Public License for more details.
*
*	 You should have received a copy of the GNU General Public License
*	 along with this program.  If not, see <http://www.gnu.org/licenses/>.
*
*****************************************************************************/


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

namespace DRAMSim {
#ifdef LOG_OUTPUT
ofstream dramsim_log;
#endif

powerCallBack_t MemorySystem::ReportPower = NULL;

MemorySystem::MemorySystem(uint id, string deviceIniFilename, string systemIniFilename, string pwd,
                           string traceFilename) :
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
	//  ranks in the system
	TOTAL_STORAGE = (long)NUM_RANKS * NUM_ROWS * NUM_COLS * NUM_BANKS * DEVICE_WIDTH / 8 * (JEDEC_DATA_BUS_WIDTH / DEVICE_WIDTH);
//	DEBUG("TOTAL_STORAGE : "<<TOTAL_STORAGE);

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
	equalsign = keyValuePair.find_first_of('=');
	overrideKey = keyValuePair.substr(0,equalsign);
	overrideVal = keyValuePair.substr(equalsign+1);
	overrideSystemParam(overrideKey, overrideVal);
}

MemorySystem::~MemorySystem()
{
	/* the MemorySystem should exist for all time, nothing should be destroying it */  
//	ERROR("MEMORY SYSTEM DESTRUCTOR with ID "<<systemID);
//	abort();

	delete(memoryController);
	ranks->clear();
	delete(ranks);
	visDataOut.flush();
	visDataOut.close();
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
	out << TOTAL_STORAGE/(1024*1024*1024) << "GB." << NUM_CHANS << "Ch." << NUM_RANKS <<"R." <<ADDRESS_MAPPING_SCHEME<<"."<<ROW_BUFFER_POLICY<<"."<< TRANS_QUEUE_DEPTH<<"TQ."<<CMD_QUEUE_DEPTH<<"CQ."<<sched<<"."<<queue;
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

void MemorySystem::RegisterCallbacks( Callback_t* readCB, Callback_t* writeCB,
                                      void (*reportPower)(double bgpower, double burstpower,
                                                          double refreshpower, double actprepower))
{
	ReturnReadData = readCB;
	WriteDataDone = writeCB;
	ReportPower = reportPower;
}

// static allocator for the library interface 
MemorySystem *getMemorySystemInstance(uint id, string dev, string sys, string pwd, string trc)
{
	return new MemorySystem(id, dev, sys, pwd, trc);
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

