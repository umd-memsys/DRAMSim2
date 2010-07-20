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
#include <sstream>

using namespace DRAMSim;
using namespace std;

ofstream cmd_verify_out; //used in Rank.cpp if VERIFICATION_OUTPUT is set

#if 0
returnCallBack_t MemorySystem::ReturnReadData = NULL;
returnCallBack_t MemorySystem::WriteDataDone = NULL;
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
	ranks = new vector<Rank>();

	for (size_t i=0; i<NUM_RANKS; i++)
	{
		Rank r = Rank();
		r.setId(i);
		r.attachMemoryController(memoryController);
		ranks->push_back(r);
	}

	if (VERIFICATION_OUTPUT)
	{
		cout << "SDFSDFSDFSDFS" << endl;
		string basefilename = deviceIniFilename.substr(deviceIniFilename.find_last_of("/")+1);
		string verify_filename =  "verilog/sim_out_"+basefilename+".tmp";
		cmd_verify_out.open(verify_filename.c_str());
		if (!cmd_verify_out)
		{
			ERROR("Cannot open "<< verify_filename);
			exit(-1);
		}
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
	equalsign = keyValuePair.find_first_of('=');
	overrideKey = keyValuePair.substr(0,equalsign);
	overrideVal = keyValuePair.substr(equalsign+1);
	overrideSystemParam(overrideKey, overrideVal);
}

MemorySystem::~MemorySystem()
{
//	DEBUG("MEMORY SYSTEM DESTRUCTOR with ID "<<systemID);
	delete(memoryController);
	ranks->clear();
	delete(ranks);
	visDataOut.flush();
	visDataOut.close();
	if (VERIFICATION_OUTPUT)
	{
		cmd_verify_out.close();
	}
}

string MemorySystem::SetOutputFileName(string traceFilename)
{
	size_t lastSlash;
	string deviceName;
	size_t deviceIniFilenameLength = deviceIniFilename.length();

	// chop off the .ini if its there
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

	if ((lastSlash = traceFilename.find_last_of("/")) != string::npos)
	{
		traceFilename = traceFilename.substr(lastSlash+1,traceFilename.length()-lastSlash-1);
	}

	string rest;
	stringstream out;

	string path = "results/";
	if (pwd.length() > 0)
	{
		path = pwd + "/" + path;
	}
	mkdirIfNotExist(path);
	path = path + traceFilename + "/";
//	path.append(traceFilename);
//	path.append("/");
	mkdirIfNotExist(path);
	path = path + deviceName + "/";
//	path.append(deviceName);
//	path.append("/");
	mkdirIfNotExist(path);
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

	out << TOTAL_STORAGE/(1024*1024*1024) << "GB." << NUM_CHANS << "Ch." << NUM_RANKS <<"R." <<ADDRESS_MAPPING_SCHEME<<"."<<ROW_BUFFER_POLICY<<"."<< TRANS_QUEUE_DEPTH<<"TQ."<<CMD_QUEUE_DEPTH<<"CQ."<<sched<<"."<<queue<<".vis";

	if (systemID!=0)
	{
		out<<"."<<systemID;
	}

	rest = out.str();
	path.append(rest);
	return path;
}

void MemorySystem::mkdirIfNotExist(string path)
{
	struct stat stat_buf;
	// dwxr-xr-x on the results directories
	mode_t mode = (S_IXOTH | S_IXGRP | S_IXUSR | S_IROTH | S_IRGRP | S_IRUSR | S_IWUSR) ;
	if (stat(path.c_str(), &stat_buf) != 0)
	{
		if (errno == ENOENT)
		{
			DEBUG("\t directory doesn't exist, trying to create ...");
			if (mkdir(path.c_str(), mode) != 0)
			{
				perror("Error Has occurred while trying to make directory: ");
				cerr << path << endl;
			}
		}
	}
	else
	{
		if (!S_ISDIR(stat_buf.st_mode))
		{
			ERROR("NOT A DIRECTORY");
			abort();
		}
		else
		{
			//DEBUG("GREAT SUCCESS!");
		}
	}
}

bool MemorySystem::WillAcceptTransaction()
{
	return memoryController->WillAcceptTransaction();
}

bool MemorySystem::addTransaction(bool isWrite, uint64_t addr)
{
	TransactionType type;
	if (isWrite)
	{
		type = DATA_WRITE;
	}
	else
	{
		type = DATA_READ;
	}

	Transaction t = Transaction(type,addr,NULL);

	return memoryController->addTransaction(t);
}

//allows CPU to make a request to the memory system
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
