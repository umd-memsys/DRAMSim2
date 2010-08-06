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




#ifndef CMDQUEUE_H
#define CMDQUEUE_H

//CommandQueue.h
//
//Header
//

#include "BusPacket.h"
#include "BankState.h"
#include "Transaction.h"
#include "SystemConfiguration.h"
#include "SimulatorObject.h"

using namespace std;

namespace DRAMSim
{
class CommandQueue : public SimulatorObject
{
public:
	//typedefs
	typedef vector<BusPacket *> BusPacket1D;
	typedef vector<BusPacket1D> BusPacket2D;
	typedef vector<BusPacket2D> BusPacket3D;

	//functions
	CommandQueue(vector< vector<BankState> > &states);
	CommandQueue();
	virtual ~CommandQueue(); 

	void enqueue(BusPacket *newBusPacket);
	bool pop(BusPacket **busPacket);
	bool hasRoomFor(uint numberToEnqueue, uint rank, uint bank);
	bool isIssuable(BusPacket *busPacket);
	bool isEmpty(uint rank);
	void needRefresh(uint rank);
	void print();
	void update(); //SimulatorObject requirement

	//fields
	
	BusPacket3D queues; // 3D array of BusPacket pointers
	vector< vector<BankState> > &bankStates;
private:

	//fields
	uint nextBank;
	uint nextRank;

	uint nextBankPRE;
	uint nextRankPRE;

	uint refreshRank;
	bool refreshWaiting;

	vector< vector<uint> > tFAWCountdown;
	vector< vector<uint> > rowAccessCounters;

	bool sendAct;
};
}

#endif

