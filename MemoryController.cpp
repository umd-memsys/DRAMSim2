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



//MemoryController.cpp
//
//Class file for memory controller object
//

#include <assert.h>
#include "ConfigIniReader.h"
#include "MemoryController.h"
#include "MemorySystem.h"
#include "AddressMapping.h"

#define SEQUENTIAL(rank,bank) (rank*cfg.NUM_BANKS)+bank

using namespace DRAMSim;

MemoryController::MemoryController(MemorySystem *parent, ostream &dramsim_log_) :
		parentMemorySystem(parent),
		cfg(parent->cfg),
		lastDumpCycle(0),
		dramsim_log(dramsim_log_),
		bankStates(cfg.NUM_RANKS, vector<BankState>(cfg.NUM_BANKS)),
		readCB(NULL), 
		writeCB(NULL),
		powerCB(NULL), 
		commandQueue(bankStates, dramsim_log_, cfg),
		poppedBusPacket(NULL),
		powerDown(cfg.NUM_RANKS,false),
		totalTransactions(0),
		grandTotalBankAccesses(cfg.NUM_RANKS*cfg.NUM_BANKS,0),
		totalReadsPerBank(cfg.NUM_RANKS*cfg.NUM_BANKS,0),
		totalWritesPerBank(cfg.NUM_RANKS*cfg.NUM_BANKS,0),
		totalReadsPerRank(cfg.NUM_RANKS,0),
		totalWritesPerRank(cfg.NUM_RANKS,0),
		refreshRank(0)
{
	//bus related fields
	outgoingCmdPacket = NULL;
	outgoingDataPacket = NULL;
	dataCyclesLeft = 0;
	cmdCyclesLeft = 0;

	//set here to avoid compile errors
	currentClockCycle = 0;

	//reserve memory for vectors
	transactionQueue.reserve(cfg.TRANS_QUEUE_DEPTH);

	writeDataCountdown.reserve(cfg.NUM_RANKS);
	writeDataToSend.reserve(cfg.NUM_RANKS);
	refreshCountdown.reserve(cfg.NUM_RANKS);

	//Power related packets
	backgroundEnergy = vector <uint64_t >(cfg.NUM_RANKS,0);
	burstEnergy = vector <uint64_t> (cfg.NUM_RANKS,0);
	actpreEnergy = vector <uint64_t> (cfg.NUM_RANKS,0);
	refreshEnergy = vector <uint64_t> (cfg.NUM_RANKS,0);

	totalEpochLatency = vector<uint64_t> (cfg.NUM_RANKS*cfg.NUM_BANKS,0);

	//staggers when each rank is due for a refresh
	for (size_t i=0;i<cfg.NUM_RANKS;i++)
	{
		refreshCountdown.push_back((int)((cfg.REFRESH_PERIOD/cfg.tCK)/cfg.NUM_RANKS)*(i+1));
	}
}

//get a bus packet from either data or cmd bus
void MemoryController::receiveFromBus(BusPacket *bpacket)
{
	if (bpacket->busPacketType != DATA)
	{
		ERROR("== Error - Memory Controller received a non-DATA bus packet from rank" << *bpacket << "\n");
		exit(0);
	}

	if (cfg.DEBUG_BUS)
	{
		PRINTN(" -- MC Receiving From Data Bus : " << *bpacket << "\n");
	}

	//add to return read data queue
	returnTransaction.push_back(new Transaction(RETURN_DATA, bpacket->physicalAddress, bpacket->data));
	totalReadsPerBank[SEQUENTIAL(bpacket->rank,bpacket->bank)]++;

	// this delete statement saves a mindboggling amount of memory
	delete(bpacket);
}

//sends read data back to the CPU
void MemoryController::returnReadData(const Transaction *trans)
{
	if (readCB != NULL)
	{
		(*readCB)(parentMemorySystem->systemID, trans->address, currentClockCycle);
	}
}

//gives the memory controller a handle on the rank objects
void MemoryController::attachRanks(vector<Rank *> *ranks)
{
	this->ranks = ranks;
}

//memory controller update
void MemoryController::update()
{

	//PRINT(" ------------------------- [" << currentClockCycle << "] -------------------------");

	//update bank states
	for (size_t i=0;i<cfg.NUM_RANKS;i++)
	{
		for (size_t j=0;j<cfg.NUM_BANKS;j++)
		{
			if (bankStates[i][j].stateChangeCountdown>0)
			{
				//decrement counters
				bankStates[i][j].stateChangeCountdown--;

				//if counter has reached 0, change state
				if (bankStates[i][j].stateChangeCountdown == 0)
				{
					switch (bankStates[i][j].lastCommand)
					{
						//only these commands have an implicit state change
					case WRITE_P:
					case READ_P:
						bankStates[i][j].currentBankState = Precharging;
						bankStates[i][j].lastCommand = PRECHARGE;
						bankStates[i][j].stateChangeCountdown = cfg.tRP;
						break;

					case REFRESH:
					case PRECHARGE:
						bankStates[i][j].currentBankState = Idle;
						break;
					default:
						break;
					}
				}
			}
		}
	}


	//check for outgoing command packets and handle countdowns
	if (outgoingCmdPacket != NULL)
	{
		cmdCyclesLeft--;
		if (cmdCyclesLeft == 0) //packet is ready to be received by rank
		{
			(*ranks)[outgoingCmdPacket->rank]->receiveFromBus(outgoingCmdPacket);
			outgoingCmdPacket = NULL;
		}
	}

	//check for outgoing data packets and handle countdowns
	if (outgoingDataPacket != NULL)
	{
		dataCyclesLeft--;
		if (dataCyclesLeft == 0)
		{
			//inform upper levels that a write is done
			if (writeCB!=NULL)
			{
				(*writeCB)(parentMemorySystem->systemID,outgoingDataPacket->physicalAddress, currentClockCycle);
			}

			(*ranks)[outgoingDataPacket->rank]->receiveFromBus(outgoingDataPacket);
			outgoingDataPacket=NULL;
		}
	}


	//if any outstanding write data needs to be sent
	//and the appropriate amount of time has passed (WL)
	//then send data on bus
	//
	//write data held in fifo vector along with countdowns
	if (writeDataCountdown.size() > 0)
	{
		for (size_t i=0;i<writeDataCountdown.size();i++)
		{
			writeDataCountdown[i]--;
		}

		if (writeDataCountdown[0]==0)
		{
			//send to bus and print debug stuff
			if (cfg.DEBUG_BUS)
			{
				PRINT(" -- MC Issuing On Data Bus    : " << *writeDataToSend[0]);
			}

			// queue up the packet to be sent
			if (outgoingDataPacket != NULL)
			{
				ERROR("== Error - Data Bus Collision");
				exit(-1);
			}

			outgoingDataPacket = writeDataToSend[0];
			dataCyclesLeft = cfg.BL/2;

			totalTransactions++;
			totalWritesPerBank[SEQUENTIAL(writeDataToSend[0]->rank,writeDataToSend[0]->bank)]++;

			writeDataCountdown.erase(writeDataCountdown.begin());
			writeDataToSend.erase(writeDataToSend.begin());
		}
	}

	//if its time for a refresh issue a refresh
	// else pop from command queue if it's not empty
	if (refreshCountdown[refreshRank]==0)
	{
		commandQueue.setRefreshNeeded(refreshRank);
		(*ranks)[refreshRank]->refreshWaiting = true;
		refreshCountdown[refreshRank] =	 cfg.REFRESH_PERIOD/cfg.tCK;
		refreshRank++;
		if (refreshRank == cfg.NUM_RANKS)
		{
			refreshRank = 0;
		}
	}
	//if a rank is powered down, make sure we power it up in time for a refresh
	else if (powerDown[refreshRank] && refreshCountdown[refreshRank] <= cfg.tXP)
	{
		(*ranks)[refreshRank]->refreshWaiting = true;
	}

	//pass a pointer to a poppedBusPacket

	//function returns true if there is something valid in poppedBusPacket
	if (commandQueue.pop(&poppedBusPacket))
	{
		if (poppedBusPacket->busPacketType == WRITE || poppedBusPacket->busPacketType == WRITE_P)
		{

			writeDataToSend.push_back(new BusPacket(DATA, poppedBusPacket->physicalAddress, poppedBusPacket->column,
			                                    poppedBusPacket->row, poppedBusPacket->rank, poppedBusPacket->bank,
			                                    poppedBusPacket->data));
			writeDataCountdown.push_back(cfg.WL);
		}

		//
		//update each bank's state based on the command that was just popped out of the command queue
		//
		//for readability's sake
		unsigned rank = poppedBusPacket->rank;
		unsigned bank = poppedBusPacket->bank;
		switch (poppedBusPacket->busPacketType)
		{
			case READ_P:
			case READ:
				//add energy to account for total
				if (cfg.DEBUG_POWER)
				{
					PRINT(" ++ Adding Read energy to total energy");
				}
				burstEnergy[rank] += (cfg.IDD4R - cfg.IDD3N) * cfg.BL/2 * cfg.NUM_DEVICES;
				if (poppedBusPacket->busPacketType == READ_P) 
				{
					//Don't bother setting next read or write times because the bank is no longer active
					//bankStates[rank][bank].currentBankState = Idle;
					bankStates[rank][bank].nextActivate = max(currentClockCycle + cfg.READ_AUTOPRE_DELAY,
							bankStates[rank][bank].nextActivate);
					bankStates[rank][bank].lastCommand = READ_P;
					bankStates[rank][bank].stateChangeCountdown = cfg.READ_TO_PRE_DELAY;
				}
				else if (poppedBusPacket->busPacketType == READ)
				{
					bankStates[rank][bank].nextPrecharge = max(currentClockCycle + cfg.READ_TO_PRE_DELAY,
							bankStates[rank][bank].nextPrecharge);
					bankStates[rank][bank].lastCommand = READ;

				}

				for (size_t i=0;i<cfg.NUM_RANKS;i++)
				{
					for (size_t j=0;j<cfg.NUM_BANKS;j++)
					{
						if (i!=poppedBusPacket->rank)
						{
							//check to make sure it is active before trying to set (save's time?)
							if (bankStates[i][j].currentBankState == RowActive)
							{
								bankStates[i][j].nextRead = max(currentClockCycle + cfg.BL/2 + cfg.tRTRS, bankStates[i][j].nextRead);
								bankStates[i][j].nextWrite = max(currentClockCycle + cfg.READ_TO_WRITE_DELAY,
										bankStates[i][j].nextWrite);
							}
						}
						else
						{
							bankStates[i][j].nextRead = max(currentClockCycle + max((unsigned)cfg.tCCD, cfg.BL/2), bankStates[i][j].nextRead);
							bankStates[i][j].nextWrite = max(currentClockCycle + cfg.READ_TO_WRITE_DELAY,
									bankStates[i][j].nextWrite);
						}
					}
				}

				if (poppedBusPacket->busPacketType == READ_P)
				{
					//set read and write to nextActivate so the state table will prevent a read or write
					//  being issued (in cq.isIssuable())before the bank state has been changed because of the
					//  auto-precharge associated with this command
					bankStates[rank][bank].nextRead = bankStates[rank][bank].nextActivate;
					bankStates[rank][bank].nextWrite = bankStates[rank][bank].nextActivate;
				}

				break;
			case WRITE_P:
			case WRITE:
				if (poppedBusPacket->busPacketType == WRITE_P) 
				{
					bankStates[rank][bank].nextActivate = max(currentClockCycle + cfg.WRITE_AUTOPRE_DELAY,
							bankStates[rank][bank].nextActivate);
					bankStates[rank][bank].lastCommand = WRITE_P;
					bankStates[rank][bank].stateChangeCountdown = cfg.WRITE_TO_PRE_DELAY;
				}
				else if (poppedBusPacket->busPacketType == WRITE)
				{
					bankStates[rank][bank].nextPrecharge = max(currentClockCycle + cfg.WRITE_TO_PRE_DELAY,
							bankStates[rank][bank].nextPrecharge);
					bankStates[rank][bank].lastCommand = WRITE;
				}


				//add energy to account for total
				if (cfg.DEBUG_POWER)
				{
					PRINT(" ++ Adding Write energy to total energy");
				}
				burstEnergy[rank] += (cfg.IDD4W - cfg.IDD3N) * cfg.BL/2 * cfg.NUM_DEVICES;

				for (size_t i=0;i<cfg.NUM_RANKS;i++)
				{
					for (size_t j=0;j<cfg.NUM_BANKS;j++)
					{
						if (i!=poppedBusPacket->rank)
						{
							if (bankStates[i][j].currentBankState == RowActive)
							{
								bankStates[i][j].nextWrite = max(currentClockCycle + cfg.BL/2 + cfg.tRTRS, bankStates[i][j].nextWrite);
								bankStates[i][j].nextRead = max(currentClockCycle + cfg.WRITE_TO_READ_DELAY_R,
										bankStates[i][j].nextRead);
							}
						}
						else
						{
							bankStates[i][j].nextWrite = max(currentClockCycle + max(cfg.BL/2, (unsigned)cfg.tCCD), bankStates[i][j].nextWrite);
							bankStates[i][j].nextRead = max(currentClockCycle + cfg.WRITE_TO_READ_DELAY_B,
									bankStates[i][j].nextRead);
						}
					}
				}

				//set read and write to nextActivate so the state table will prevent a read or write
				//  being issued (in cq.isIssuable())before the bank state has been changed because of the
				//  auto-precharge associated with this command
				if (poppedBusPacket->busPacketType == WRITE_P)
				{
					bankStates[rank][bank].nextRead = bankStates[rank][bank].nextActivate;
					bankStates[rank][bank].nextWrite = bankStates[rank][bank].nextActivate;
				}

				break;
			case ACTIVATE:
				//add energy to account for total
				if (cfg.DEBUG_POWER)
				{
					PRINT(" ++ Adding Activate and Precharge energy to total energy");
				}
				actpreEnergy[rank] += ((cfg.IDD0 * cfg.tRC) - ((cfg.IDD3N * cfg.tRAS) + (cfg.IDD2N * (cfg.tRC - cfg.tRAS)))) * cfg.NUM_DEVICES;

				bankStates[rank][bank].currentBankState = RowActive;
				bankStates[rank][bank].lastCommand = ACTIVATE;
				bankStates[rank][bank].openRowAddress = poppedBusPacket->row;
				bankStates[rank][bank].nextActivate = max(currentClockCycle + cfg.tRC, bankStates[rank][bank].nextActivate);
				bankStates[rank][bank].nextPrecharge = max(currentClockCycle + cfg.tRAS, bankStates[rank][bank].nextPrecharge);

				//if we are using posted-CAS, the next column access can be sooner than normal operation

				bankStates[rank][bank].nextRead = max(currentClockCycle + (cfg.tRCD-cfg.AL), bankStates[rank][bank].nextRead);
				bankStates[rank][bank].nextWrite = max(currentClockCycle + (cfg.tRCD-cfg.AL), bankStates[rank][bank].nextWrite);

				for (size_t i=0;i<cfg.NUM_BANKS;i++)
				{
					if (i!=poppedBusPacket->bank)
					{
						bankStates[rank][i].nextActivate = max(currentClockCycle + cfg.tRRD, bankStates[rank][i].nextActivate);
					}
				}

				break;
			case PRECHARGE:
				bankStates[rank][bank].currentBankState = Precharging;
				bankStates[rank][bank].lastCommand = PRECHARGE;
				bankStates[rank][bank].stateChangeCountdown = cfg.tRP;
				bankStates[rank][bank].nextActivate = max(currentClockCycle + cfg.tRP, bankStates[rank][bank].nextActivate);

				break;
			case REFRESH:
				//add energy to account for total
				if (cfg.DEBUG_POWER)
				{
					PRINT(" ++ Adding Refresh energy to total energy");
				}
				refreshEnergy[rank] += (cfg.IDD5 - cfg.IDD3N) * cfg.tRFC * cfg.NUM_DEVICES;

				for (size_t i=0;i<cfg.NUM_BANKS;i++)
				{
					bankStates[rank][i].nextActivate = currentClockCycle + cfg.tRFC;
					bankStates[rank][i].currentBankState = Refreshing;
					bankStates[rank][i].lastCommand = REFRESH;
					bankStates[rank][i].stateChangeCountdown = cfg.tRFC;
				}

				break;
			default:
				ERROR("== Error - Popped a command we shouldn't have of type : " << poppedBusPacket->busPacketType);
				exit(0);
		}

		//issue on bus and print debug
		if (cfg.DEBUG_BUS)
		{
			PRINTN(" -- MC Issuing On Command Bus : " << *poppedBusPacket << "\n");
		}

		//check for collision on bus
		if (outgoingCmdPacket != NULL)
		{
			ERROR("== Error - Command Bus Collision");
			exit(-1);
		}
		outgoingCmdPacket = poppedBusPacket;
		cmdCyclesLeft = cfg.tCMD;

	}

	for (size_t i=0;i<transactionQueue.size();i++)
	{
		//pop off top transaction from queue
		//
		//	assuming simple scheduling at the moment
		//	will eventually add policies here
		Transaction *transaction = transactionQueue[i];

		//map address to rank,bank,row,col
		unsigned newTransactionChan, newTransactionRank, newTransactionBank, newTransactionRow, newTransactionColumn;

		// pass these in as references so they get set by the addressMapping function
		addressMapping(transaction->address, newTransactionChan, newTransactionRank, newTransactionBank, newTransactionRow, newTransactionColumn, cfg);

		//if we have room, break up the transaction into the appropriate commands
		//and add them to the command queue
		if (commandQueue.hasRoomFor(2, newTransactionRank, newTransactionBank))
		{
			if (cfg.DEBUG_ADDR_MAP) 
			{
				PRINTN("== New Transaction - Mapping Address [0x" << hex << transaction->address << dec << "]");
				if (transaction->transactionType == DATA_READ) 
				{
					PRINT(" (Read)");
				}
				else
				{
					PRINT(" (Write)");
				}
				PRINT("  Rank : " << newTransactionRank);
				PRINT("  Bank : " << newTransactionBank);
				PRINT("  Row  : " << newTransactionRow);
				PRINT("  Col  : " << newTransactionColumn);
			}



			//now that we know there is room in the command queue, we can remove from the transaction queue
			transactionQueue.erase(transactionQueue.begin()+i);

			//create activate command to the row we just translated
			BusPacket *ACTcommand = new BusPacket(ACTIVATE, transaction->address,
					newTransactionColumn, newTransactionRow, newTransactionRank,
					newTransactionBank, 0);

			//create read or write command and enqueue it
			BusPacketType bpType = transaction->getBusPacketType(cfg);
			BusPacket *command = new BusPacket(bpType, transaction->address,
					newTransactionColumn, newTransactionRow, newTransactionRank,
					newTransactionBank, transaction->data);



			commandQueue.enqueue(ACTcommand);
			commandQueue.enqueue(command);

			// If we have a read, save the transaction so when the data comes back
			// in a bus packet, we can staple it back into a transaction and return it
			if (transaction->transactionType == DATA_READ)
			{
				pendingReadTransactions.push_back(transaction);
			}
			else
			{
				// just delete the transaction now that it's a buspacket
				delete transaction; 
			}
			/* only allow one transaction to be scheduled per cycle -- this should
			 * be a reasonable assumption considering how much logic would be
			 * required to schedule multiple entries per cycle (parallel data
			 * lines, switching logic, decision logic)
			 */
			break;
		}
		else // no room, do nothing this cycle
		{
			//PRINT( "== Warning - No room in command queue" << endl;
		}
	}


	//calculate power
	//  this is done on a per-rank basis, since power characterization is done per device (not per bank)
	for (size_t i=0;i<cfg.NUM_RANKS;i++)
	{
		if (cfg.USE_LOW_POWER)
		{
			//if there are no commands in the queue and that particular rank is not waiting for a refresh...
			if (commandQueue.isEmpty(i) && !(*ranks)[i]->refreshWaiting)
			{
				//check to make sure all banks are idle
				bool allIdle = true;
				for (size_t j=0;j<cfg.NUM_BANKS;j++)
				{
					if (bankStates[i][j].currentBankState != Idle)
					{
						allIdle = false;
						break;
					}
				}

				//if they ARE all idle, put in power down mode and set appropriate fields
				if (allIdle)
				{
					powerDown[i] = true;
					(*ranks)[i]->powerDown();
					for (size_t j=0;j<cfg.NUM_BANKS;j++)
					{
						bankStates[i][j].currentBankState = PowerDown;
						bankStates[i][j].nextPowerUp = currentClockCycle + cfg.tCKE;
					}
				}
			}
			//if there IS something in the queue or there IS a refresh waiting (and we can power up), do it
			else if (currentClockCycle >= bankStates[i][0].nextPowerUp && powerDown[i]) //use 0 since theyre all the same
			{
				powerDown[i] = false;
				(*ranks)[i]->powerUp();
				for (size_t j=0;j<cfg.NUM_BANKS;j++)
				{
					bankStates[i][j].currentBankState = Idle;
					bankStates[i][j].nextActivate = currentClockCycle + cfg.tXP;
				}
			}
		}

		//check for open bank
		bool bankOpen = false;
		for (size_t j=0;j<cfg.NUM_BANKS;j++)
		{
			if (bankStates[i][j].currentBankState == Refreshing ||
			        bankStates[i][j].currentBankState == RowActive)
			{
				bankOpen = true;
				break;
			}
		}

		//background power is dependent on whether or not a bank is open or not
		if (bankOpen)
		{
			if (cfg.DEBUG_POWER)
			{
				PRINT(" ++ Adding IDD3N to total energy [from rank "<< i <<"]");
			}
			backgroundEnergy[i] += cfg.IDD3N * cfg.NUM_DEVICES;
		}
		else
		{
			//if we're in power-down mode, use the correct current
			if (powerDown[i])
			{
				if (cfg.DEBUG_POWER)
				{
					PRINT(" ++ Adding IDD2P to total energy [from rank " << i << "]");
				}
				backgroundEnergy[i] += cfg.IDD2P * cfg.NUM_DEVICES;
			}
			else
			{
				if (cfg.DEBUG_POWER)
				{
					PRINT(" ++ Adding IDD2N to total energy [from rank " << i << "]");
				}
				backgroundEnergy[i] += cfg.IDD2N * cfg.NUM_DEVICES;
			}
		}
	}

	//check for outstanding data to return to the CPU
	if (returnTransaction.size()>0)
	{
		if (cfg.DEBUG_BUS)
		{
			PRINTN(" -- MC Issuing to CPU bus : " << *returnTransaction[0]);
		}
		totalTransactions++;

		bool foundMatch=false;
		//find the pending read transaction to calculate latency
		for (size_t i=0;i<pendingReadTransactions.size();i++)
		{
			if (pendingReadTransactions[i]->address == returnTransaction[0]->address)
			{
				//if(currentClockCycle - pendingReadTransactions[i]->timeAdded > 2000)
				//	{
				//		pendingReadTransactions[i]->print();
				//		exit(0);
				//	}
				unsigned chan,rank,bank,row,col;
				addressMapping(returnTransaction[0]->address,chan,rank,bank,row,col, cfg);
				insertHistogram(currentClockCycle-pendingReadTransactions[i]->timeAdded,rank,bank);
				//return latency
				returnReadData(pendingReadTransactions[i]);

				delete pendingReadTransactions[i];
				pendingReadTransactions.erase(pendingReadTransactions.begin()+i);
				foundMatch=true; 
				break;
			}
		}
		if (!foundMatch)
		{
			ERROR("Can't find a matching transaction for 0x"<<hex<<returnTransaction[0]->address<<dec);
			abort(); 
		}
		delete returnTransaction[0];
		returnTransaction.erase(returnTransaction.begin());
	}

	//decrement refresh counters
	for (size_t i=0;i<cfg.NUM_RANKS;i++)
	{
		refreshCountdown[i]--;
	}

	//
	//print debug
	//
	if (cfg.DEBUG_TRANS_Q)
	{
		PRINT("== Printing transaction queue");
		for (size_t i=0;i<transactionQueue.size();i++)
		{
			PRINTN("  " << i << "] "<< *transactionQueue[i]);
		}
	}

	if (cfg.DEBUG_BANKSTATE)
	{
		//TODO: move this to BankState.cpp
		PRINT("== Printing bank states (According to MC)");
		for (size_t i=0;i<cfg.NUM_RANKS;i++)
		{
			for (size_t j=0;j<cfg.NUM_BANKS;j++)
			{
				if (bankStates[i][j].currentBankState == RowActive)
				{
					PRINTN("[" << bankStates[i][j].openRowAddress << "] ");
				}
				else if (bankStates[i][j].currentBankState == Idle)
				{
					PRINTN("[idle] ");
				}
				else if (bankStates[i][j].currentBankState == Precharging)
				{
					PRINTN("[pre] ");
				}
				else if (bankStates[i][j].currentBankState == Refreshing)
				{
					PRINTN("[ref] ");
				}
				else if (bankStates[i][j].currentBankState == PowerDown)
				{
					PRINTN("[lowp] ");
				}
			}
			PRINT(""); // effectively just cout<<endl;
		}
	}

	if (cfg.DEBUG_CMD_Q)
	{
		commandQueue.print();
	}

	commandQueue.step();

}

bool MemoryController::WillAcceptTransaction()
{
	return transactionQueue.size() < cfg.TRANS_QUEUE_DEPTH;
}

//allows outside source to make request of memory system
bool MemoryController::addTransaction(Transaction *trans)
{
	if (WillAcceptTransaction())
	{
		trans->timeAdded = currentClockCycle;
		transactionQueue.push_back(trans);
		return true;
	}
	else 
	{
		return false;
	}
}

void MemoryController::resetStats()
{
	for (size_t i=0; i<cfg.NUM_RANKS; i++)
	{
		for (size_t j=0; j<cfg.NUM_BANKS; j++)
		{
			//XXX: this means the bank list won't be printed for partial epochs
			grandTotalBankAccesses[SEQUENTIAL(i,j)] += totalReadsPerBank[SEQUENTIAL(i,j)] + totalWritesPerBank[SEQUENTIAL(i,j)];
			totalReadsPerBank[SEQUENTIAL(i,j)] = 0;
			totalWritesPerBank[SEQUENTIAL(i,j)] = 0;
			totalEpochLatency[SEQUENTIAL(i,j)] = 0;
		}

		burstEnergy[i] = 0;
		actpreEnergy[i] = 0;
		refreshEnergy[i] = 0;
		backgroundEnergy[i] = 0;
		totalReadsPerRank[i] = 0;
		totalWritesPerRank[i] = 0;
	}
}
//prints statistics at the end of an epoch or  simulation
void MemoryController::printStats(CSVWriter *CSVOut, bool finalStats)
{
	unsigned myChannel = parentMemorySystem->systemID;

	//if we are not at the end of the epoch, make sure to adjust for the actual number of cycles elapsed

	uint64_t cyclesElapsed = currentClockCycle - lastDumpCycle; //(currentClockCycle % EPOCH_LENGTH == 0) ? EPOCH_LENGTH : currentClockCycle % EPOCH_LENGTH;
	unsigned bytesPerTransaction = (cfg.JEDEC_DATA_BUS_BITS*cfg.BL)/8;
	uint64_t totalBytesTransferred = totalTransactions * bytesPerTransaction;
	double secondsThisEpoch = (double)cyclesElapsed * cfg.tCK * 1E-9;

	// only per rank
	vector<double> backgroundPower = vector<double>(cfg.NUM_RANKS,0.0);
	vector<double> burstPower = vector<double>(cfg.NUM_RANKS,0.0);
	vector<double> refreshPower = vector<double>(cfg.NUM_RANKS,0.0);
	vector<double> actprePower = vector<double>(cfg.NUM_RANKS,0.0);
	vector<double> averagePower = vector<double>(cfg.NUM_RANKS,0.0);

	// per bank variables
	vector<double> averageLatency = vector<double>(cfg.NUM_RANKS*cfg.NUM_BANKS,0.0);
	vector<double> bandwidth = vector<double>(cfg.NUM_RANKS*cfg.NUM_BANKS,0.0);

	double totalBandwidth=0.0;
	for (size_t i=0;i<cfg.NUM_RANKS;i++)
	{
		for (size_t j=0; j<cfg.NUM_BANKS; j++)
		{
			bandwidth[SEQUENTIAL(i,j)] = (((double)(totalReadsPerBank[SEQUENTIAL(i,j)]+totalWritesPerBank[SEQUENTIAL(i,j)]) * (double)bytesPerTransaction)/(1024.0*1024.0*1024.0)) / secondsThisEpoch;
			averageLatency[SEQUENTIAL(i,j)] = ((float)totalEpochLatency[SEQUENTIAL(i,j)] / (float)(totalReadsPerBank[SEQUENTIAL(i,j)])) * cfg.tCK;
			totalBandwidth+=bandwidth[SEQUENTIAL(i,j)];
			totalReadsPerRank[i] += totalReadsPerBank[SEQUENTIAL(i,j)];
			totalWritesPerRank[i] += totalWritesPerBank[SEQUENTIAL(i,j)];
		}
	}
#ifdef LOG_OUTPUT
	dramsim_log.precision(3);
	dramsim_log.setf(ios::fixed,ios::floatfield);
#else
	cout.precision(3);
	cout.setf(ios::fixed,ios::floatfield);
#endif

	PRINT( " =======================================================" );
	PRINT( " ============== Printing Statistics [id:"<<parentMemorySystem->systemID<<"]==============" );
	PRINTN( "   Total Return Transactions : " << totalTransactions );
	PRINT( " ("<<totalBytesTransferred <<" bytes) aggregate average bandwidth "<<totalBandwidth<<"GB/s");

	double totalAggregateBandwidth = 0.0;	
	for (size_t r=0;r<cfg.NUM_RANKS;r++)
	{
		PRINT( "      -Rank   "<<r<<" : ");
		PRINTN( "        -Reads  : " << totalReadsPerRank[r]);
		PRINT( " ("<<totalReadsPerRank[r] * bytesPerTransaction<<" bytes)");
		PRINTN( "        -Writes : " << totalWritesPerRank[r]);
		PRINT( " ("<<totalWritesPerRank[r] * bytesPerTransaction<<" bytes)");
		for (size_t j=0;j<cfg.NUM_BANKS;j++)
		{
			PRINT( "        -Bandwidth / Latency  (Bank " <<j<<"): " <<bandwidth[SEQUENTIAL(r,j)] << " GB/s\t\t" <<averageLatency[SEQUENTIAL(r,j)] << " ns");
		}

		// factor of 1000 at the end is to account for the fact that totalEnergy is accumulated in mJ since IDD values are given in mA
		backgroundPower[r] = ((double)backgroundEnergy[r] / (double)(cyclesElapsed)) * cfg.Vdd / 1000.0;
		burstPower[r] = ((double)burstEnergy[r] / (double)(cyclesElapsed)) * cfg.Vdd / 1000.0;
		refreshPower[r] = ((double) refreshEnergy[r] / (double)(cyclesElapsed)) * cfg.Vdd / 1000.0;
		actprePower[r] = ((double)actpreEnergy[r] / (double)(cyclesElapsed)) * cfg.Vdd / 1000.0;
		averagePower[r] = ((backgroundEnergy[r] + burstEnergy[r] + refreshEnergy[r] + actpreEnergy[r]) / (double)cyclesElapsed) * cfg.Vdd / 1000.0;

		if (powerCB != NULL)
		{
			(*powerCB)(backgroundPower[r],burstPower[r],refreshPower[r],actprePower[r]);
		}

		PRINT( " == Power Data for Rank        " << r );
		PRINT( "   Average Power (watts)     : " << averagePower[r] );
		PRINT( "     -Background (watts)     : " << backgroundPower[r] );
		PRINT( "     -Act/Pre    (watts)     : " << actprePower[r] );
		PRINT( "     -Burst      (watts)     : " << burstPower[r]);
		PRINT( "     -Refresh    (watts)     : " << refreshPower[r] );
		// FIXME: oh man, the manipulations with csvOut are really terrible
		if (cfg.VIS_FILE_OUTPUT)
		{
			//	cout << "c="<<myChannel<< " r="<<r<<"writing to csv out on cycle "<< currentClockCycle<<endl;
			// write the vis file output
			if (CSVOut) {
				CSVWriter &csvOut = *CSVOut; 
				csvOut << CSVWriter::IndexedName("Background_Power",myChannel,r) <<backgroundPower[r];
				csvOut << CSVWriter::IndexedName("ACT_PRE_Power",myChannel,r) << actprePower[r];
				csvOut << CSVWriter::IndexedName("Burst_Power",myChannel,r) << burstPower[r];
				csvOut << CSVWriter::IndexedName("Refresh_Power",myChannel,r) << refreshPower[r];
				double totalRankBandwidth=0.0;
				for (size_t b=0; b<cfg.NUM_BANKS; b++)
				{
					csvOut << CSVWriter::IndexedName("Bandwidth",myChannel,r,b) << bandwidth[SEQUENTIAL(r,b)];
					totalRankBandwidth += bandwidth[SEQUENTIAL(r,b)];
					totalAggregateBandwidth += bandwidth[SEQUENTIAL(r,b)];
					csvOut << CSVWriter::IndexedName("Average_Latency",myChannel,r,b) << averageLatency[SEQUENTIAL(r,b)];
				}
				csvOut << CSVWriter::IndexedName("Rank_Aggregate_Bandwidth",myChannel,r) << totalRankBandwidth; 
				csvOut << CSVWriter::IndexedName("Rank_Average_Bandwidth",myChannel,r) << totalRankBandwidth/cfg.NUM_RANKS; 
			}
		}
	}
	if (cfg.VIS_FILE_OUTPUT)
	{
		if (CSVOut) {
			CSVWriter &csvOut = *CSVOut; 

			csvOut << CSVWriter::IndexedName("Aggregate_Bandwidth",myChannel) << totalAggregateBandwidth;
			csvOut << CSVWriter::IndexedName("Average_Bandwidth",myChannel) << totalAggregateBandwidth / (cfg.NUM_RANKS*cfg.NUM_BANKS);
		}
	}

	// only print the latency histogram at the end of the simulation since it clogs the output too much to print every epoch
	if (finalStats)
	{
		PRINT( " ---  Latency list ("<<latencies.size()<<")");
		PRINT( "       [lat] : #");
		/*
		if (cfg.VIS_FILE_OUTPUT)
		{
			csvOut.getOutputStream() << "!!HISTOGRAM_DATA"<<endl;
		}
		*/

		map<unsigned,unsigned>::iterator it; //
		for (it=latencies.begin(); it!=latencies.end(); it++)
		{
			PRINT( "       ["<< it->first <<"-"<<it->first+(cfg.LATENCY_HISTOGRAM_BIN_SIZE-1)<<"] : "<< it->second );
			/*
			if (cfg.VIS_FILE_OUTPUT)
			{
				csvOut.getOutputStream() << it->first <<"="<< it->second << endl;
			}
			*/
		}
		if (currentClockCycle % cfg.EPOCH_LENGTH == 0)
		{
			PRINT( " --- Grand Total Bank usage list");
			for (size_t i=0;i<cfg.NUM_RANKS;i++)
			{
				PRINT("Rank "<<i<<":"); 
				for (size_t j=0;j<cfg.NUM_BANKS;j++)
				{
					PRINT( "  b"<<j<<": "<<grandTotalBankAccesses[SEQUENTIAL(i,j)]);
				}
			}
		}

	}


	PRINT(endl<< " == Pending Transactions : "<<pendingReadTransactions.size()<<" ("<<currentClockCycle<<")==");
	/*
	for(size_t i=0;i<pendingReadTransactions.size();i++)
		{
			PRINT( i << "] I've been waiting for "<<currentClockCycle-pendingReadTransactions[i].timeAdded<<endl;
		}
	*/
#ifdef LOG_OUTPUT
	dramsim_log.flush();
#endif

	lastDumpCycle = currentClockCycle;
	resetStats();
}
MemoryController::~MemoryController()
{
	for (size_t i=0; i<ranks->size(); i++) {
		delete (*ranks)[i];
	}
	//ERROR("MEMORY CONTROLLER DESTRUCTOR");
	//abort();
	for (size_t i=0; i<pendingReadTransactions.size(); i++)
	{
		delete pendingReadTransactions[i];
	}
	for (size_t i=0; i<returnTransaction.size(); i++)
	{
		delete returnTransaction[i];
	}

}
void MemoryController::registerCallbacks(TransactionCompleteCB *readCB_, TransactionCompleteCB *writeCB_, PowerCallback_t *powerCB_) {
	readCB = readCB_; 
	writeCB = writeCB_;
	powerCB = powerCB_; 
}
//inserts a latency into the latency histogram
void MemoryController::insertHistogram(unsigned latencyValue, unsigned rank, unsigned bank)
{
	totalEpochLatency[SEQUENTIAL(rank,bank)] += latencyValue;
	//poor man's way to bin things.
	latencies[(latencyValue/cfg.LATENCY_HISTOGRAM_BIN_SIZE)*cfg.LATENCY_HISTOGRAM_BIN_SIZE]++;
}
