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






#include "IniReader.h"

using namespace std;

// these are the values that are extern'd in SystemConfig.h so that they
// have global scope even though they are set by IniReader

uint64_t TOTAL_STORAGE;
uint NUM_BANKS;
uint NUM_RANKS;
uint NUM_CHANS;
uint NUM_ROWS;
uint NUM_COLS;
uint DEVICE_WIDTH;

uint REFRESH_PERIOD;
float tCK;
uint CL;
uint AL;
uint BL;
uint tRAS;
uint tRCD;
uint tRRD;
uint tRC;
uint tRP;
uint tCCD;
uint tRTP;
uint tWTR;
uint tWR;
uint tRTRS;
uint tRFC;
uint tFAW;
uint tCKE;
uint tXP;
uint tCMD;

uint IDD0;
uint IDD1;
uint IDD2P;
uint IDD2Q;
uint IDD2N;
uint IDD3Pf;
uint IDD3Ps;
uint IDD3N;
uint IDD4W;
uint IDD4R;
uint IDD5;
uint IDD6;
uint IDD6L;
uint IDD7;


//in bytes
uint CACHE_LINE_SIZE; //4

uint JEDEC_DATA_BUS_WIDTH;

//Memory Controller related parameters
uint TRANS_QUEUE_DEPTH;
uint CMD_QUEUE_DEPTH;

//cycles within an epoch
uint EPOCH_COUNT;

//row accesses allowed before closing (open page)
uint TOTAL_ROW_ACCESSES;

// strings and their associated enums
string ROW_BUFFER_POLICY;
string SCHEDULING_POLICY;
string ADDRESS_MAPPING_SCHEME;
string QUEUING_STRUCTURE;

bool DEBUG_TRANS_Q;
bool DEBUG_CMD_Q;
bool DEBUG_ADDR_MAP;
bool DEBUG_BANKSTATE;
bool DEBUG_BUS;
bool DEBUG_BANKS;
bool DEBUG_POWER;
bool USE_LOW_POWER;

bool VERIFICATION_OUTPUT;

bool DEBUG_INI_READER=false;

namespace DRAMSim
{
RowBufferPolicy rowBufferPolicy;
SchedulingPolicy schedulingPolicy;
AddressMappingScheme addressMappingScheme;
QueuingStructure queuingStructure;


//Map the string names to the variables they set
static ConfigMap configMap[] =
{
	//DEFINE_UINT_PARAM -- see IniReader.h
	DEFINE_UINT_PARAM(NUM_BANKS,DEV_PARAM),
	DEFINE_UINT_PARAM(NUM_ROWS,DEV_PARAM),
	DEFINE_UINT_PARAM(NUM_COLS,DEV_PARAM),
	DEFINE_UINT_PARAM(DEVICE_WIDTH,DEV_PARAM),
	DEFINE_UINT_PARAM(REFRESH_PERIOD,DEV_PARAM),
	DEFINE_FLOAT_PARAM(tCK,DEV_PARAM),
	DEFINE_UINT_PARAM(CL,DEV_PARAM),
	DEFINE_UINT_PARAM(AL,DEV_PARAM),
	DEFINE_UINT_PARAM(BL,DEV_PARAM),
	DEFINE_UINT_PARAM(tRAS,DEV_PARAM),
	DEFINE_UINT_PARAM(tRCD,DEV_PARAM),
	DEFINE_UINT_PARAM(tRRD,DEV_PARAM),
	DEFINE_UINT_PARAM(tRC,DEV_PARAM),
	DEFINE_UINT_PARAM(tRP,DEV_PARAM),
	DEFINE_UINT_PARAM(tCCD,DEV_PARAM),
	DEFINE_UINT_PARAM(tRTP,DEV_PARAM),
	DEFINE_UINT_PARAM(tWTR,DEV_PARAM),
	DEFINE_UINT_PARAM(tWR,DEV_PARAM),
	DEFINE_UINT_PARAM(tRTRS,DEV_PARAM),
	DEFINE_UINT_PARAM(tRFC,DEV_PARAM),
	DEFINE_UINT_PARAM(tFAW,DEV_PARAM),
	DEFINE_UINT_PARAM(tCKE,DEV_PARAM),
	DEFINE_UINT_PARAM(tXP,DEV_PARAM),
	DEFINE_UINT_PARAM(tCMD,DEV_PARAM),
	DEFINE_UINT_PARAM(IDD0,DEV_PARAM),
	DEFINE_UINT_PARAM(IDD1,DEV_PARAM),
	DEFINE_UINT_PARAM(IDD2P,DEV_PARAM),
	DEFINE_UINT_PARAM(IDD2Q,DEV_PARAM),
	DEFINE_UINT_PARAM(IDD2N,DEV_PARAM),
	DEFINE_UINT_PARAM(IDD3Pf,DEV_PARAM),
	DEFINE_UINT_PARAM(IDD3Ps,DEV_PARAM),
	DEFINE_UINT_PARAM(IDD3N,DEV_PARAM),
	DEFINE_UINT_PARAM(IDD4W,DEV_PARAM),
	DEFINE_UINT_PARAM(IDD4R,DEV_PARAM),
	DEFINE_UINT_PARAM(IDD5,DEV_PARAM),
	DEFINE_UINT_PARAM(IDD6,DEV_PARAM),
	DEFINE_UINT_PARAM(IDD6L,DEV_PARAM),
	DEFINE_UINT_PARAM(IDD7,DEV_PARAM),

	//DEFINE_UINT64_PARAM(TOTAL_STORAGE,SYS_PARAM),
	DEFINE_UINT_PARAM(NUM_RANKS,SYS_PARAM),
	DEFINE_UINT_PARAM(NUM_CHANS,SYS_PARAM),
	DEFINE_UINT_PARAM(CACHE_LINE_SIZE,SYS_PARAM),
	DEFINE_UINT_PARAM(JEDEC_DATA_BUS_WIDTH,SYS_PARAM),
	//Memory Controller related parameters
	DEFINE_UINT_PARAM(TRANS_QUEUE_DEPTH,SYS_PARAM),
	DEFINE_UINT_PARAM(CMD_QUEUE_DEPTH,SYS_PARAM),
	//Power
	DEFINE_BOOL_PARAM(USE_LOW_POWER,SYS_PARAM),
	DEFINE_UINT_PARAM(EPOCH_COUNT,SYS_PARAM),
	DEFINE_UINT_PARAM(TOTAL_ROW_ACCESSES,SYS_PARAM),
	DEFINE_STRING_PARAM(ROW_BUFFER_POLICY,SYS_PARAM),
	DEFINE_STRING_PARAM(SCHEDULING_POLICY,SYS_PARAM),
	DEFINE_STRING_PARAM(ADDRESS_MAPPING_SCHEME,SYS_PARAM),
	DEFINE_STRING_PARAM(QUEUING_STRUCTURE,SYS_PARAM),
	// debug flags
	DEFINE_BOOL_PARAM(DEBUG_TRANS_Q,SYS_PARAM),
	DEFINE_BOOL_PARAM(DEBUG_CMD_Q,SYS_PARAM),
	DEFINE_BOOL_PARAM(DEBUG_ADDR_MAP,SYS_PARAM),
	DEFINE_BOOL_PARAM(DEBUG_BANKSTATE,SYS_PARAM),
	DEFINE_BOOL_PARAM(DEBUG_BUS,SYS_PARAM),
	DEFINE_BOOL_PARAM(DEBUG_BANKS,SYS_PARAM),
	DEFINE_BOOL_PARAM(DEBUG_POWER,SYS_PARAM),
	//modelsim output mode?
	DEFINE_BOOL_PARAM(VERIFICATION_OUTPUT,SYS_PARAM),
	{"", NULL, UINT, SYS_PARAM, false} // tracer value to signify end of list; if you delete it, epic fail will result
};

void IniReader::WriteValuesOut(std::ofstream &visDataOut)
{
	//DEBUG("WRITE CALLED");
	visDataOut<<"!!SYSTEM_INI"<<endl;
	for (size_t i=0; configMap[i].variablePtr != NULL; i++)
	{
		if (configMap[i].parameterType == SYS_PARAM)
		{
			visDataOut<<configMap[i].iniKey<<"=";
			switch (configMap[i].variableType)
			{
				//parse and set each type of variable
			case UINT:
				visDataOut << *((uint *)configMap[i].variablePtr);
				break;
			case UINT64:
				visDataOut << *((uint64_t *)configMap[i].variablePtr);
				break;
			case FLOAT:
				visDataOut << *((float *)configMap[i].variablePtr);
				break;
			case STRING:
				visDataOut << *((string *)configMap[i].variablePtr);
				break;
			case BOOL:
				if (*((bool *)configMap[i].variablePtr))
				{
					visDataOut <<"true";
				}
				else
				{
					visDataOut <<"false";
				}
				break;
			}
			visDataOut << endl;
		}
	}

	visDataOut<<"!!DEVICE_INI"<<endl;
	for (size_t i=0; configMap[i].variablePtr != NULL; i++)
	{
		if (configMap[i].parameterType == DEV_PARAM)
		{
			visDataOut<<configMap[i].iniKey<<"=";
			switch (configMap[i].variableType)
			{
				//parse and set each type of variable
			case UINT:
				visDataOut << *((uint *)configMap[i].variablePtr);
				break;
			case UINT64:
				visDataOut << *((uint64_t *)configMap[i].variablePtr);
				break;

			case FLOAT:
				visDataOut << *((float *)configMap[i].variablePtr);
				break;
			case STRING:
				visDataOut << *((string *)configMap[i].variablePtr);
				break;
			case BOOL:
				if (*((bool *)configMap[i].variablePtr))
				{
					visDataOut <<"true";
				}
				else
				{
					visDataOut <<"false";
				}
				break;
			}
			visDataOut << endl;
		}
	}
	visDataOut<<"!!EPOCH_DATA"<<endl;

}

void IniReader::SetKey(string key, string valueString, bool isSystemParam, size_t lineNumber)
{
	size_t i;
	uint intValue;
	uint64_t int64Value;
	float floatValue;

	for (i=0; configMap[i].variablePtr != NULL; i++)
	{
		istringstream iss(valueString);
		// match up the string in the config map with the key we parsed
		if (key.compare(configMap[i].iniKey) == 0)
		{
			switch (configMap[i].variableType)
			{
				//parse and set each type of variable
			case UINT:
				if ((iss >> dec >> intValue).fail())
				{
					ERROR("could not parse line "<<lineNumber<<" (non-numeric value '"<<valueString<<"')?");
				}
				*((uint *)configMap[i].variablePtr) = intValue;
				if (DEBUG_INI_READER)
				{
					DEBUG("\t - SETTING "<<configMap[i].iniKey<<"="<<intValue);
				}
				break;
			case UINT64:
				if ((iss >> dec >> int64Value).fail())
				{
					ERROR("could not parse line "<<lineNumber<<" (non-numeric value '"<<valueString<<"')?");
				}
				*((uint64_t *)configMap[i].variablePtr) = int64Value;
				if (DEBUG_INI_READER)
				{
					DEBUG("\t - SETTING "<<configMap[i].iniKey<<"="<<int64Value);
				}
				break;
			case FLOAT:
				if ((iss >> dec >> floatValue).fail())
				{
					ERROR("could not parse line "<<lineNumber<<" (non-numeric value '"<<valueString<<"')?");
				}
				*((float *)configMap[i].variablePtr) = floatValue;
				if (DEBUG_INI_READER)
				{
					DEBUG("\t - SETTING "<<configMap[i].iniKey<<"="<<floatValue);
				}
				break;
			case STRING:
				*((string *)configMap[i].variablePtr) = string(valueString);
				if (DEBUG_INI_READER)
				{
					DEBUG("\t - SETTING "<<configMap[i].iniKey<<"="<<valueString);
				}

				break;
			case BOOL:
				if (valueString == "true" || valueString == "1")
				{
					*((bool *)configMap[i].variablePtr) = true;
				}
				else
				{
					*((bool *)configMap[i].variablePtr) = false;
				}
			}
			// lineNumber == 0 implies that this is an override parameter from the command line, so don't bother doing these checks
			if (lineNumber > 0)
			{
				if (isSystemParam && configMap[i].parameterType == DEV_PARAM)
				{
					DEBUG("WARNING: Found device parameter "<<configMap[i].iniKey<<" in system config file");
				}
				else if (!isSystemParam && configMap[i].parameterType == SYS_PARAM)
				{
					DEBUG("WARNING: Found system parameter "<<configMap[i].iniKey<<" in device config file");
				}
			}
			// use the pointer stored in the config map to set the value of the variable
			// to make sure all parameters are in the ini file
			configMap[i].wasSet = true;
			break;
		}
	}

	if (configMap[i].variablePtr == NULL)
	{
		DEBUG("WARNING: UNKNOWN KEY '"<<key<<"' IN INI FILE");
	}
}

void IniReader::ReadIniFile(string filename, bool isSystemFile)
{
	ifstream iniFile;
	string line;
	string key,valueString;

	size_t commentIndex, equalsIndex;
	size_t lineNumber=0;

	iniFile.open(filename.c_str());
	if (iniFile.is_open())
	{
		while (!iniFile.eof())
		{
			lineNumber++;
			getline(iniFile, line);
			//this can happen if the filename is actually a directory
			if (iniFile.bad())
			{
				ERROR("Cannot read ini file '"<<filename<<"'");
				exit(-1);
			}
			// skip zero-length lines
			if (line.size() == 0)
			{
//					DEBUG("Skipping blank line "<<lineNumber);
				continue;
			}
			//search for a comment char
			if ((commentIndex = line.find_first_of(";")) != string::npos)
			{
				//if the comment char is the first char, ignore the whole line
				if (commentIndex == 0)
				{
//						DEBUG("Skipping comment line "<<lineNumber);
					continue;
				}
//					DEBUG("Truncating line at comment"<<line[commentIndex-1]);
				//truncate the line at first comment before going on
				line = line.substr(0,commentIndex);
			}
			// trim off the end spaces that might have been between the value and comment char
			size_t whiteSpaceEndIndex;
			if ((whiteSpaceEndIndex = line.find_last_not_of(" \t")) != string::npos)
			{
				line = line.substr(0,whiteSpaceEndIndex+1);
			}

			// at this point line should be a valid, commentless string

			// a line has to have an equals sign
			if ((equalsIndex = line.find_first_of("=")) == string::npos)
			{
				ERROR("Malformed Line "<<lineNumber<<" (missing equals)");
				abort();
			}
			size_t strlen = line.size();
			// all characters before the equals are the key
			key = line.substr(0, equalsIndex);
			// all characters after the equals are the value
			valueString = line.substr(equalsIndex+1,strlen-equalsIndex);

			IniReader::SetKey(key, valueString, lineNumber, isSystemFile);
			// got to the end of the config map without finding the key
		}
	}
	else
	{
		ERROR ("Unable to load ini file "<<filename);
		abort();
	}
}

void IniReader::OverrideKeys(vector<string> keys, vector<string>values)
{
	if (keys.size() != values.size())
	{
		ERROR("-o option is messed up");
		exit(-1);
	}
	for (size_t i=0; i<keys.size(); i++)
	{
		IniReader::SetKey(keys[i], values[i]);
	}
}

bool IniReader::CheckIfAllSet()
{
	// check to make sure all parameters that we exepected were set
	for (size_t i=0; configMap[i].variablePtr != NULL; i++)
	{
		if (!configMap[i].wasSet)
		{
			DEBUG("WARNING: KEY "<<configMap[i].iniKey<<" NOT FOUND IN INI FILE.");
			switch (configMap[i].variableType)
			{
				//the string and bool values can be defaulted, but generally we need all the numeric values to be set to continue
			case UINT:
			case UINT64:
			case FLOAT:
				ERROR("Cannot continue without key '"<<configMap[i].iniKey<<"' set.");
				return false;
				break;
			case BOOL:
				*((bool *)configMap[i].variablePtr) = false;
				DEBUG("\tSetting Default: "<<configMap[i].iniKey<<"=false");
				break;
			case STRING:
				break;
			}
		}
	}
	return true;
}
void IniReader::InitEnumsFromStrings()
{
	if (ADDRESS_MAPPING_SCHEME == "scheme1")
	{
		addressMappingScheme = Scheme1;
		if (DEBUG_INI_READER) 
		{
			DEBUG("ADDR SCHEME: 1");
		}
	}
	else if (ADDRESS_MAPPING_SCHEME == "scheme2")
	{
		addressMappingScheme = Scheme2;
		if (DEBUG_INI_READER) 
		{
			DEBUG("ADDR SCHEME: 2");
		}
	}
	else if (ADDRESS_MAPPING_SCHEME == "scheme3")
	{
		addressMappingScheme = Scheme3;
		if (DEBUG_INI_READER) 
		{
			DEBUG("ADDR SCHEME: 3");
		}
	}
	else if (ADDRESS_MAPPING_SCHEME == "scheme4")
	{
		addressMappingScheme = Scheme4;
		if (DEBUG_INI_READER) 
		{
			DEBUG("ADDR SCHEME: 4");
		}
	}
	else if (ADDRESS_MAPPING_SCHEME == "scheme5")
	{
		addressMappingScheme = Scheme5;
		if (DEBUG_INI_READER) 
		{
			DEBUG("ADDR SCHEME: 5");
		}
	}
	else if (ADDRESS_MAPPING_SCHEME == "scheme6")
	{
		addressMappingScheme = Scheme6;
		if (DEBUG_INI_READER) 
		{
			DEBUG("ADDR SCHEME: 6");
		}
	}
	else
	{
		cout << "WARNING: unknown address mapping scheme '"<<ADDRESS_MAPPING_SCHEME<<"'; valid values are 'scheme1', 'scheme2', 'scheme3', 'scheme4', 'scheme5'. Defaulting to scheme1"<<endl;
		addressMappingScheme = Scheme1;
	}

	if (ROW_BUFFER_POLICY == "open_page")
	{
		rowBufferPolicy = OpenPage;
		if (DEBUG_INI_READER) 
		{
			DEBUG("ROW BUFFER: open page");
		}
	}
	else if (ROW_BUFFER_POLICY == "close_page")
	{
		rowBufferPolicy = ClosePage;
		if (DEBUG_INI_READER) 
		{
			DEBUG("ROW BUFFER: close page");
		}
	}
	else
	{
		cout << "WARNING: unknown row buffer policy '"<<ROW_BUFFER_POLICY<<"'; valid values are 'open_page' or 'close_page', Defaulting to Close Page."<<endl;
		rowBufferPolicy = ClosePage;
	}

	if (QUEUING_STRUCTURE == "per_rank_per_bank")
	{
		queuingStructure = PerRankPerBank;
		if (DEBUG_INI_READER) 
		{
			DEBUG("QUEUING STRUCT: per rank per bank");
		}
	}
	else if (QUEUING_STRUCTURE == "per_rank")
	{
		queuingStructure = PerRank;
		if (DEBUG_INI_READER) 
		{
			DEBUG("QUEUING STRUCT: per rank");
		}
	}
	else
	{
		cout << "WARNING: Unknown queueing structure '"<<QUEUING_STRUCTURE<<"'; valid options are 'per_rank' and 'per_rank_per_bank', defaulting to Per Rank Per Bank"<<endl;
		queuingStructure = PerRankPerBank;
	}

	if (SCHEDULING_POLICY == "rank_then_bank_round_robin")
	{
		schedulingPolicy = RankThenBankRoundRobin;
		if (DEBUG_INI_READER) 
		{
			DEBUG("SCHEDULING: Rank Then Bank");
		}
	}
	else if (SCHEDULING_POLICY == "bank_then_rank_round_robin")
	{
		schedulingPolicy = BankThenRankRoundRobin;
		if (DEBUG_INI_READER) 
		{
			DEBUG("SCHEDULING: Bank Then Rank");
		}
	}
	else
	{
		cout << "WARNING: Unknown scheduling policy '"<<SCHEDULING_POLICY<<"'; valid options are 'rank_then_bank_round_robin' or 'bank_then_rank_round_robin'; defaulting to Bank Then Rank Round Robin" << endl;
		schedulingPolicy = BankThenRankRoundRobin;
	}

}

#if 0
// Wrote it, but did not use it -- might be handy in the future
void IniReader::Trim(string &str)
{
	size_t begin,end;
	if ((begin = str.find_first_not_of(" ")) == string::npos)
	{
		begin = 0;
	}
	if ((end = str.find_last_not_of(" ")) == string::npos)
	{
		end = str.size()-1;
	}
	str = str.substr(begin,end-begin+1);
}
#endif

}
