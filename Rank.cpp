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

#include "ConfigIniReader.h"
#include "Rank.h"
#include "MemoryController.h"

using namespace std;
using namespace DRAMSim;

Rank::Rank(MemoryController &memoryController_, ostream &dramsim_log_) :
	memoryController(memoryController_),
	cfg(memoryController.cfg),
	id(-1),
	dramsim_log(dramsim_log_),
	isPowerDown(false),
	refreshWaiting(false),
	readReturnCountdown(0),
	banks(cfg.NUM_BANKS, Bank(cfg, dramsim_log_)),
	bankStates(cfg.NUM_BANKS, cfg)

{
	outgoingDataPacket = NULL;
	dataCyclesLeft = 0;
}

// mutators
void Rank::setId(int id)
{
	this->id = id;
}

Rank::~Rank()
{
	readReturnPacket.clear(); 
}

bool Rank::isTimingValid(BusPacket *packet) const {
	const BankState &bankState = bankStates[packet->bank];
	switch (packet->busPacketType) {
		case READ: 
		return !(bankState.currentBankState != RowActive ||
		        currentClockCycle < bankState.nextRead ||
		        packet->row != bankState.openRowAddress);

		case READ_P: 
			return !(bankState.currentBankState != RowActive ||
		        currentClockCycle < bankState.nextRead ||
		        packet->row != bankState.openRowAddress);

		case WRITE: 
			return !(bankState.currentBankState != RowActive ||
		        currentClockCycle < bankState.nextWrite ||
		        packet->row != bankState.openRowAddress);

		case WRITE_P:
			return !(bankState.currentBankState != RowActive ||
		        currentClockCycle < bankState.nextWrite ||
		        packet->row != bankState.openRowAddress);

		case ACTIVATE:
			return !(bankState.currentBankState != Idle ||
		        currentClockCycle < bankState.nextActivate);

		case PRECHARGE:
			return !(bankState.currentBankState != RowActive ||
		        currentClockCycle < bankState.nextPrecharge);

		case REFRESH: 
			for (size_t i=0;i<cfg.NUM_BANKS;i++)
			{
				if (bankStates[i].currentBankState != Idle)
					return false;
			}
			return true;

			break; // not strictly necessary
		case DATA: 
			// TODO: implement a check
			return true;

		default:
			ERROR("What is this?"<<*packet);
			abort();
	}
	// should not be reachable
	abort();
}

void Rank::receiveFromBus(BusPacket *packet)
{
	// First, make sure this packet is allowed at the current cycle
	if (!isTimingValid(packet)) {
		ERROR(currentClockCycle << ": Got "<<*packet<<" it wasn't allowed: "<< bankStates[packet->bank]);
		abort();
	}

	if (cfg.DEBUG_BUS) {
		PRINT(" -- R" << this->id << " Receiving On Bus    : " << *packet);
	}

	if (cfg.VERIFICATION_OUTPUT) {
		packet->print(currentClockCycle,false);
	}

	// Update the current rank's bank state
	bankStates[packet->bank].updateState(*packet, currentClockCycle);

	// update the other bank states and 
	switch (packet->busPacketType)
	{
		case READ:
		case READ_P:
			//update state table
			for (size_t i=0;i<cfg.NUM_BANKS;i++)
			{
				bankStates[i].nextRead = max(bankStates[i].nextRead, currentClockCycle + max((unsigned)cfg.tCCD, (unsigned)cfg.BL/2));
				bankStates[i].nextWrite = max(bankStates[i].nextWrite, currentClockCycle + cfg.READ_TO_WRITE_DELAY);
			}

			//get the read data and put it in the storage which delays until the appropriate time (RL)
			banks[packet->bank].read(packet);
			packet->busPacketType = DATA;

			readReturnPacket.push_back(packet);
			readReturnCountdown.push_back(cfg.RL);
			break;

		case WRITE:
		case WRITE_P:
			for (size_t i=0;i<cfg.NUM_BANKS;i++) {
				bankStates[i].nextRead = max(bankStates[i].nextRead, currentClockCycle + cfg.WRITE_TO_READ_DELAY_B);
				bankStates[i].nextWrite = max(bankStates[i].nextWrite, currentClockCycle + max((unsigned)cfg.BL/2, (unsigned)cfg.tCCD));
			}
			BusPacketPool.dealloc(packet);
			break;

		case ACTIVATE:
			for (size_t i=0;i<cfg.NUM_BANKS;i++) {
				if (i != packet->bank) {
					bankStates[i].nextActivate = max(bankStates[i].nextActivate, currentClockCycle + cfg.tRRD);
				}
			}
			BusPacketPool.dealloc(packet);
			break;

		case PRECHARGE:
			BusPacketPool.dealloc(packet);
			break;

		case REFRESH:
			refreshWaiting = false;
			for (size_t i=0;i<cfg.NUM_BANKS;i++)
			{
				bankStates[i].nextActivate = currentClockCycle + cfg.tRFC;
			}
			BusPacketPool.dealloc(packet);
			break;
		case DATA:
			// TODO: replace this check with something that works?
			/*
				if(packet->bank != incomingWriteBank ||
				packet->row != incomingWriteRow ||
				packet->column != incomingWriteColumn)
				{
				cout << "== Error - Rank " << id << " received a DATA packet to the wrong place" << endl;
				packet->print();
				bankStates[packet->bank].print();
				exit(0);
				}
				*/
			banks[packet->bank].write(packet);
			// end of the line for the write packet
			BusPacketPool.dealloc(packet);
			break;

		default:
			ERROR("== Error - Unknown BusPacketType trying to be sent to Bank");
			exit(0);
			break;
	}
}

int Rank::getId() const
{
	return this->id;
}

void Rank::update()
{
	for (size_t b=0; b<bankStates.size(); ++b) {
		bankStates[b].updateStateChange();
	}

	// An outgoing packet is one that is currently sending on the bus
	// do the book keeping for the packet's time left on the bus
	if (outgoingDataPacket != NULL)
	{
		dataCyclesLeft--;
		if (dataCyclesLeft == 0)
		{
			//if the packet is done on the bus, call receiveFromBus and free up the bus
			memoryController.receiveFromBus(outgoingDataPacket);
			outgoingDataPacket = NULL;
		}
	}

	// decrement the counter for all packets waiting to be sent back
	for (size_t i=0;i<readReturnCountdown.size();i++)
	{
		readReturnCountdown[i]--;
	}


	if (!readReturnCountdown.empty() && readReturnCountdown[0]==0)
	{
		// RL time has passed since the read was issued; this packet is
		// ready to go out on the bus

		outgoingDataPacket = readReturnPacket[0];
		dataCyclesLeft = cfg.BL/2;

		// remove the packet from the ranks
		readReturnPacket.erase(readReturnPacket.begin());
		readReturnCountdown.erase(readReturnCountdown.begin());

		if (cfg.DEBUG_BUS)
		{
			PRINT(" -- R" << this->id << " Issuing On Data Bus : " << *outgoingDataPacket);
		}

	}
}

//power down the rank
void Rank::powerDown()
{
	//perform checks
	for (size_t i=0;i<cfg.NUM_BANKS;i++)
	{
		if (bankStates[i].currentBankState != Idle)
		{
			ERROR("== Error - Trying to power down rank " << id << " while not all banks are idle");
			exit(0);
		}

		bankStates[i].nextPowerUp = currentClockCycle + cfg.tCKE;
		bankStates[i].currentBankState = PowerDown;
	}

	isPowerDown = true;
}

//power up the rank
void Rank::powerUp()
{
	if (!isPowerDown)
	{
		ERROR("== Error - Trying to power up rank " << id << " while it is not already powered down");
		exit(0);
	}

	isPowerDown = false;

	for (size_t i=0;i<cfg.NUM_BANKS;i++)
	{
		if (bankStates[i].nextPowerUp > currentClockCycle)
		{
			ERROR("== Error - Trying to power up rank " << id << " before we're allowed to");
			ERROR(bankStates[i].nextPowerUp << "    " << currentClockCycle);
			exit(0);
		}
		bankStates[i].nextActivate = currentClockCycle + cfg.tXP;
		bankStates[i].currentBankState = Idle;
	}
}
