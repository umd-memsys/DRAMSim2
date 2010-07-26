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

#ifndef SYSCONFIG_H
#define SYSCONFIG_H

#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <cstdlib>
#include <stdint.h>
#include "PrintMacros.h"

//SystemConfiguration.h
//
//Configuration values for the current system



//number of latencies per bucket in the latency histogram
//TODO: move to system ini file
#define HISTOGRAM_BIN_SIZE 10

extern std::ofstream cmd_verify_out; //used by BusPacket.cpp if VERIFICATION_OUTPUT is enabled
//extern std::ofstream visDataOut;

extern bool VERIFICATION_OUTPUT; // output suitable to feed to modelsim

extern bool DEBUG_TRANS_Q;
extern bool DEBUG_CMD_Q;
extern bool DEBUG_ADDR_MAP;
extern bool DEBUG_BANKSTATE;
extern bool DEBUG_BUS;
extern bool DEBUG_BANKS;
extern bool DEBUG_POWER;
extern bool USE_LOW_POWER;

extern uint64_t TOTAL_STORAGE;
extern uint NUM_BANKS;
extern uint NUM_RANKS;
extern uint NUM_CHANS;
extern uint NUM_ROWS;
extern uint NUM_COLS;
extern uint DEVICE_WIDTH;

//in nanoseconds
extern uint REFRESH_PERIOD;
extern float tCK;

extern uint CL;
extern uint AL;
#define RL (CL+AL)
#define WL (RL-1)
extern uint BL;
extern uint tRAS;
extern uint tRCD;
extern uint tRRD;
extern uint tRC;
extern uint tRP;
extern uint tCCD;
extern uint tRTP;
extern uint tWTR;
extern uint tWR;
extern uint tRTRS;
extern uint tRFC;
extern uint tFAW;
extern uint tCKE;
extern uint tXP;

extern uint tCMD;

extern uint IDD0;
extern uint IDD1;
extern uint IDD2P;
extern uint IDD2Q;
extern uint IDD2N;
extern uint IDD3Pf;
extern uint IDD3Ps;
extern uint IDD3N;
extern uint IDD4W;
extern uint IDD4R;
extern uint IDD5;
extern uint IDD6;
extern uint IDD6L;
extern uint IDD7;

//same bank
#define READ_TO_PRE_DELAY (AL+BL/2+max(((int)tRTP),2)-2)
#define WRITE_TO_PRE_DELAY (WL+BL/2+tWR)
#define READ_TO_WRITE_DELAY (RL+BL/2+tRTRS-WL)
#define READ_AUTOPRE_DELAY (AL+tRTP+tRP)
#define WRITE_AUTOPRE_DELAY (WL+BL/2+tWR+tRP)
#define WRITE_TO_READ_DELAY_B (WL+BL/2+tWTR) //interbank
#define WRITE_TO_READ_DELAY_R (WL+BL/2+tRTRS-RL) //interrank

//in bytes
extern uint CACHE_LINE_SIZE; //4

extern uint JEDEC_DATA_BUS_WIDTH;

//Memory Controller related parameters
extern uint TRANS_QUEUE_DEPTH;
extern uint CMD_QUEUE_DEPTH;

extern uint EPOCH_COUNT;

extern uint TOTAL_ROW_ACCESSES;

extern std::string ROW_BUFFER_POLICY;
extern std::string SCHEDULING_POLICY;
extern std::string ADDRESS_MAPPING_SCHEME;
extern std::string QUEUING_STRUCTURE;

enum TraceType
{
	k6,
	mase,
	misc
};

enum AddressMappingScheme
{
	Scheme1,
	Scheme2,
	Scheme3,
	Scheme4,
	Scheme5,
	Scheme6
};

// used in MemoryController and CommandQueue
enum RowBufferPolicy
{
	OpenPage,
	ClosePage
};

// Only used in CommandQueue
enum QueuingStructure
{
	PerRank,
	PerRankPerBank
};

enum SchedulingPolicy
{
	RankThenBankRoundRobin,
	BankThenRankRoundRobin
};


// set by IniReader.cpp


namespace DRAMSim
{
typedef void (*returnCallBack_t)(uint id, uint64_t addr, uint64_t clockcycle);
typedef void (*powerCallBack_t)(double bgpower, double burstpower, double refreshpower, double actprepower);

extern RowBufferPolicy rowBufferPolicy;
extern SchedulingPolicy schedulingPolicy;
extern AddressMappingScheme addressMappingScheme;
extern QueuingStructure queuingStructure;
//
//FUNCTIONS
//

uint inline dramsim_log2(unsigned value)
{
	uint logbase2 = 0;
	unsigned orig = value;
	value>>=1;
	while (value>0)
	{
		value >>= 1;
		logbase2++;
	}
	if ((uint)1<<logbase2<orig)logbase2++;
	return logbase2;
}

};

#endif

