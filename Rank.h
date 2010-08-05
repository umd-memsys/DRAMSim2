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


#ifndef RANK_H
#define RANK_H

#include "SimulatorObject.h"
#include "BusPacket.h"
#include "SystemConfiguration.h"
#include "Bank.h"
#include "BankState.h"

using namespace std;
using namespace DRAMSim;

namespace DRAMSim
{
class MemoryController; //forward declaration
class Rank : public SimulatorObject
{
private:
	int id;
	uint incomingWriteBank;
	uint incomingWriteRow;
	uint incomingWriteColumn;
	bool isPowerDown;

public:
	//functions
	Rank();
	void receiveFromBus(BusPacket *packet);
	void attachMemoryController(MemoryController *mc);
	int getId() const;
	void setId(int id);
	void update();
	void powerUp();
	void powerDown();

	//fields
	vector<Bank> banks;
	MemoryController *memoryController;
	BusPacket *outgoingDataPacket;
	uint dataCyclesLeft;
	bool refreshWaiting;

	//these are vectors so that each element is per-bank
	vector<BusPacket *> readReturnPacket;
	vector<uint> readReturnCountdown;
	vector<BankState> bankStates;
};
}
#endif

