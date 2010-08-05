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






#ifndef MEMORYCONTROLLER_H
#define MEMORYCONTROLLER_H

//MemoryController.h
//
//Header file for memory controller object
//

#include "SimulatorObject.h"
#include "Transaction.h"
#include "SystemConfiguration.h"
#include "CommandQueue.h"
#include "BusPacket.h"
#include "BankState.h"
#include "Rank.h"
#include <map>

using namespace std;
using namespace DRAMSim;

namespace DRAMSim
{
class MemorySystem;
class MemoryController : public SimulatorObject
{

public:
	//functions
	MemoryController(MemorySystem* ms, std::ofstream *outfile);
	virtual ~MemoryController();

	bool addTransaction(Transaction &trans);
	bool WillAcceptTransaction();
	void returnReadData(const Transaction &trans);
	void receiveFromBus(BusPacket *bpacket);
	void attachRanks(vector<Rank> *ranks);
	void update();
	void printStats(bool finalStats = false);


	//fields
	vector<Transaction> transactionQueue;
	vector< vector <BankState> > bankStates;
private:
	//functions
	void addressMapping(uint64_t physicalAddress, uint &rank, uint &bank, uint &row, uint &col);
	void insertHistogram(uint latencyValue, uint rank, uint bank);

	//fields
	MemorySystem *parentMemorySystem;

	CommandQueue commandQueue;
	BusPacket *poppedBusPacket;
	vector<uint>refreshCountdown;
	vector<BusPacket *> writeDataToSend;
	vector<uint> writeDataCountdown;
	vector<Transaction> returnTransaction;
	vector<Transaction> pendingReadTransactions;
	map<uint,uint> latencies; // latencyValue -> latencyCount
	vector<bool> powerDown;

	vector<Rank> *ranks;

	//output file
	std::ofstream *visDataOut;

	// these packets are counting down waiting to be transmitted on the "bus"
	BusPacket *outgoingCmdPacket;
	uint cmdCyclesLeft;
	BusPacket *outgoingDataPacket;
	uint dataCyclesLeft;

	uint64_t totalTransactions;
	vector<uint64_t> totalReadsPerBank;
	vector<uint64_t> totalWritesPerBank;

	vector<uint64_t> totalReadsPerRank;
	vector<uint64_t> totalWritesPerRank;

	// energy values are per rank
	vector< uint64_t > backgroundEnergy;
	vector< uint64_t > burstEnergy;
	vector< uint64_t > actpreEnergy;
	vector< uint64_t > refreshEnergy;

	vector< uint64_t > totalEpochLatency;

	uint channelBitWidth;
	uint rankBitWidth;
	uint bankBitWidth;
	uint rowBitWidth;
	uint colBitWidth;
	uint byteOffsetWidth;


	uint refreshRank;

	uint NUM_DEVICES;
};
}

#endif

