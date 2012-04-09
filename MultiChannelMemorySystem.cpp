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
#include <errno.h> 
#include <sstream> //stringstream
#include <stdlib.h> // getenv()
// for directory operations 
#include <sys/stat.h>
#include <sys/types.h>

#include "MultiChannelMemorySystem.h"
#include "AddressMapping.h"
#include "IniReader.h"



using namespace DRAMSim; 


MultiChannelMemorySystem::MultiChannelMemorySystem(const string &deviceIniFilename_, const string &systemIniFilename_, const string &pwd_, const string &traceFilename_, unsigned megsOfMemory_)
	:megsOfMemory(megsOfMemory_), deviceIniFilename(deviceIniFilename_), systemIniFilename(systemIniFilename_), traceFilename(traceFilename_), pwd(pwd_)
{
	if (!isPowerOfTwo(megsOfMemory))
	{
		ERROR("Please specify a power of 2 memory size"); 
		abort(); 
	}

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

	IniReader::InitEnumsFromStrings();
	if (!IniReader::CheckIfAllSet())
	{
		exit(-1);
	}

	if (NUM_CHANS == 0) 
	{
		ERROR("Zero channels"); 
		abort(); 
	}

	for (size_t i=0; i<NUM_CHANS; i++)
	{
		MemorySystem *channel = new MemorySystem(i, megsOfMemory/NUM_CHANS, visDataOut);
		channels.push_back(channel);
	}

}
/**
 * This function creates up to 3 output files: 
 * 	- The .log file if LOG_OUTPUT is set
 * 	- the .vis file where csv data for each epoch will go
 * 	- the .tmp file if verification output is enabled
 * The results directory is setup to be in PWD/TRACEFILENAME.[SIM_DESC]/DRAM_PARTNAME/PARAMS.vis
 * The environment variable SIM_DESC is also appended to output files/directories
 *
 * TODO: verification info needs to be generated per channel so it has to be
 * moved back to MemorySystem
 **/
void MultiChannelMemorySystem::InitOutputFiles(string traceFilename)
{
	size_t lastSlash;
	size_t deviceIniFilenameLength = deviceIniFilename.length();
	string sim_description_str;
	string deviceName;
	
	char *sim_description = getenv("SIM_DESC");
	if (sim_description)
	{
			sim_description_str = string(sim_description);
	}


	// create a properly named verification output file if need be and open it
	// as the stream 'cmd_verify_out'
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
			abort(); 
		}
	}
	// This sets up the vis file output along with the creating the result
	// directory structure if it doesn't exist
	if (VIS_FILE_OUTPUT)
	{
		// chop off the .ini if it's there
		if (deviceIniFilename.substr(deviceIniFilenameLength-4) == ".ini")
		{
			deviceName = deviceIniFilename.substr(0,deviceIniFilenameLength-4);
			deviceIniFilenameLength -= 4;
		}

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
		stringstream out,tmpNum;

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

		//filename so far, without extension, see if it exists already
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

		path.append(filename);
		cerr << "writing vis file to " <<path<<endl;

		visDataOut.open(path.c_str());
		if (!visDataOut)
		{
			ERROR("Cannot open '"<<path<<"'");
			exit(-1);
		}
		//write out the ini config values for the visualizer tool
		IniReader::WriteValuesOut(visDataOut);

	}
#ifdef LOG_OUTPUT
	string dramsimLogFilename("dramsim");
	if (sim_description != NULL)
	{
		dramsimLogFilename += "."+sim_description_str; 
	}
	dramsimLogFilename += ".log";

	dramsim_log.open(dramsimLogFilename.c_str(), ios_base::out | ios_base::trunc );

	if (!dramsim_log) 
	{
	ERROR("Cannot open "<< dramsimLogFilename);
	//	exit(-1); 
	}
#endif

}

bool MultiChannelMemorySystem::fileExists(string path)
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

void MultiChannelMemorySystem::mkdirIfNotExist(string path)
{
	struct stat stat_buf;
	// check if the directory exists
	if (stat(path.c_str(), &stat_buf) != 0) // nonzero return value on error, check errno
	{
		if (errno == ENOENT) 
		{
//			DEBUG("\t directory doesn't exist, trying to create ...");

			// set permissions dwxr-xr-x on the results directories
			mode_t mode = (S_IXOTH | S_IXGRP | S_IXUSR | S_IROTH | S_IRGRP | S_IRUSR | S_IWUSR) ;
			if (mkdir(path.c_str(), mode) != 0)
			{
				perror("Error Has occurred while trying to make directory: ");
				cerr << path << endl;
				abort();
			}
		}
		else
		{
			perror("Something else when wrong: "); 
			abort();
		}
	}
	else // directory already exists
	{
		if (!S_ISDIR(stat_buf.st_mode))
		{
			ERROR(path << "is not a directory");
			abort();
		}
	}
}

void MultiChannelMemorySystem::overrideSystemParam(string key, string value)
{
	cerr << "Override key " <<key<<"="<<value<<endl;
	IniReader::SetKey(key, value, true);
}

void MultiChannelMemorySystem::overrideSystemParam(string keyValuePair)
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

MultiChannelMemorySystem::~MultiChannelMemorySystem()
{
// Should only ever be called on exit, so don't bother to delete stuff, just
// flush our streams and close em up
#ifdef LOG_OUTPUT
	dramsim_log.flush();
	dramsim_log.close();
#endif
	if (VIS_FILE_OUTPUT) 
	{	
		visDataOut.flush();
		visDataOut.close();
	}
}
void MultiChannelMemorySystem::update() 
{
	if (currentClockCycle == 0)
	{
		InitOutputFiles(traceFilename);
	}

	for (size_t i=0; i<NUM_CHANS; i++)
	{
		channels[i]->update(); 
	}
	currentClockCycle++; 
}
unsigned MultiChannelMemorySystem::findChannelNumber(uint64_t addr)
{
	// Single channel case is a trivial shortcut case 
	if (NUM_CHANS == 1)
	{
		return 0; 
	}

	if (!isPowerOfTwo(NUM_CHANS))
	{
		ERROR("We can only support power of two # of channels.\n" <<
				"I don't know what Intel was thinking, but trying to address map half a bit is a neat trick that we're not sure how to do"); 
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
	return channels[channelNumber]->addTransaction(trans); 
}

bool MultiChannelMemorySystem::addTransaction(bool isWrite, uint64_t addr)
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

bool MultiChannelMemorySystem::willAcceptTransaction(uint64_t addr)
{
	unsigned chan, rank,bank,row,col; 
	addressMapping(addr, chan, rank, bank, row, col); 
	return channels[chan]->WillAcceptTransaction(); 
}

bool MultiChannelMemorySystem::willAcceptTransaction()
{
	for (size_t c=0; c<NUM_CHANS; c++) {
		if (!channels[c]->WillAcceptTransaction())
		{
			return false; 
		}
	}
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
MultiChannelMemorySystem *getMemorySystemInstance(const string &dev, const string &sys, const string &pwd, const string &trc, unsigned megsOfMemory) 
{
	return new MultiChannelMemorySystem(dev, sys, pwd, trc, megsOfMemory);
}
}
