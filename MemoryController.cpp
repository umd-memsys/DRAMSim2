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

//MemoryController.cpp
//
//Class file for memory controller object
//

#include "MemoryController.h"
#include "MemorySystem.h"

#define SEQUENTIAL(rank,bank) (rank*NUM_BANKS)+bank

using namespace DRAMSim;

MemoryController::MemoryController(MemorySystem *parent, std::ofstream *outfile) :
		commandQueue (CommandQueue(bankStates)),
		poppedBusPacket(NULL),
		totalTransactions(0),
		channelBitWidth (dramsim_log2(NUM_CHANS)),
		rankBitWidth (dramsim_log2(NUM_RANKS)),
		bankBitWidth (dramsim_log2(NUM_BANKS)),
		rowBitWidth (dramsim_log2(NUM_ROWS)),
		colBitWidth (dramsim_log2(NUM_COLS)),
		byteOffsetWidth (dramsim_log2(CACHE_LINE_SIZE)),
		refreshRank(0)
{
	//get handle on parent
	parentMemorySystem = parent;
	visDataOut = outfile;

	//calculate number of devices
	/************************
	  This code has always been problematic even though it's pretty simple. I'll try to explain it 
	  for my own sanity. 

	  There are two main variables here that we could let the user choose:
	  NUM_RANKS or TOTAL_STORAGE.  Since the density and width of the part is
	  fixed by the device ini file, the only variable that is really
	  controllable is the number of ranks. Users care more about choosing the
	  total amount of storage, but with a fixed device they might choose a total
	  storage that isn't possible. In that sense it's not as good to allow them
	  to choose TOTAL_STORAGE (because any NUM_RANKS value >1 will be valid).

	  However, users don't care (or know) about ranks, they care about total
	  storage, so maybe it's better to let them choose and just throw an error
	  if they choose something invalid. 

	  A bit of background: 

	  Each column contains DEVICE_WIDTH bits. A row contains NUM_COLS columns.
	  Each bank contains NUM_ROWS rows. Therefore, the total storage per DRAM device is: 
	  		PER_DEVICE_STORAGE = NUM_ROWS*NUM_COLS*DEVICE_WIDTH*NUM_BANKS (in bits)

	 A rank *must* have a 64 bit output bus (JEDEC standard), so each rank must have:
	  		NUM_DEVICES_PER_RANK = 64/DEVICE_WIDTH  
	 
	If we multiply these two numbers to get the storage per rank (in bits), we get:
			PER_RANK_STORAGE = PER_DEVICE_STORAGE*NUM_DEVICES_PER_RANK = NUM_ROWS*NUM_COLS*NUM_BANKS*64 

	Finally, to get TOTAL_STORAGE, we need to multiply by NUM_RANKS
			TOTAL_STORAGE = PER_RANK_STORAGE*NUM_RANKS (total storage in bits)

	So one could compute this in reverse -- compute NUM_DEVICES,
	PER_DEVICE_STORAGE, and PER_RANK_STORAGE first since all these parameters
	are set by the device ini. Then, TOTAL_STORAGE/PER_RANK_STORAGE = NUM_RANKS 

	The only way this could run into problems is if TOTAL_STORAGE < PER_RANK_STORAGE,
	which could happen for very dense parts.
	*********************/
	//TODO: 

	//uint64_t total_storage64 = (uint64_t)TOTAL_STORAGE;
	//NUM_DEVICES = (total_storage64*8) / (NUM_ROWS * NUM_COLS * DEVICE_WIDTH * NUM_BANKS);
	NUM_DEVICES = ((TOTAL_STORAGE * 8) / ((long)NUM_ROWS * NUM_COLS * DEVICE_WIDTH * NUM_BANKS))/NUM_RANKS;

	DEBUG ("NUM DEVICES="<<NUM_DEVICES);

	//bus related fields
	outgoingCmdPacket = NULL;
	outgoingDataPacket = NULL;
	dataCyclesLeft = 0;
	cmdCyclesLeft = 0;

	//set here to avoid compile errors
	currentClockCycle = 0;

	//reserve memory for vectors
	transactionQueue.reserve(TRANS_QUEUE_DEPTH);
	bankStates = vector< vector <BankState> >(NUM_RANKS, vector<BankState>(NUM_BANKS));
	powerDown = vector<bool>(NUM_RANKS,false);
	totalReadsPerBank = vector<uint64_t>(NUM_RANKS*NUM_BANKS,0);
	totalWritesPerBank = vector<uint64_t>(NUM_RANKS*NUM_BANKS,0);
	totalReadsPerRank = vector<uint64_t>(NUM_RANKS,0);
	totalWritesPerRank = vector<uint64_t>(NUM_RANKS,0);

	writeDataCountdown.reserve(NUM_RANKS);
	writeDataToSend.reserve(NUM_RANKS);
	refreshCountdown.reserve(NUM_RANKS);

	//Power related packets
	backgroundEnergy = vector <uint64_t >(NUM_RANKS,0);
	burstEnergy = vector <uint64_t> (NUM_RANKS,0);
	actpreEnergy = vector <uint64_t> (NUM_RANKS,0);
	refreshEnergy = vector <uint64_t> (NUM_RANKS,0);

	totalEpochLatency = vector<uint64_t> (NUM_RANKS*NUM_BANKS,0);

	//staggers when each rank is due for a refresh
	for (size_t i=0;i<NUM_RANKS;i++)
	{
		refreshCountdown.push_back((int)((REFRESH_PERIOD/tCK)/NUM_RANKS)*(i+1));
	}
}

//get a bus packet from either data or cmd bus
void MemoryController::receiveFromBus(BusPacket *bpacket)
{
	if (bpacket->busPacketType != DATA)
	{
		ERROR("== Error - Memory Controller received a non-DATA bus packet from rank");
		bpacket->print();
		exit(0);
	}

	if (DEBUG_BUS)
	{
		PRINTN(" -- MC Receiving From Data Bus : ");
		bpacket->print();
	}

	//add to return read data queue
	returnTransaction.push_back(Transaction(RETURN_DATA, bpacket->physicalAddress, bpacket->data));
	totalReadsPerBank[SEQUENTIAL(bpacket->rank,bpacket->bank)]++;

	// this delete statement saves a mindboggling amount of memory
	delete(bpacket);
}

//sends read data back to the CPU
void MemoryController::returnReadData(const Transaction &trans)
{
	if (parentMemorySystem->ReturnReadData!=NULL)
	{
		(*parentMemorySystem->ReturnReadData)(parentMemorySystem->systemID, trans.address, currentClockCycle);
	}
}

//gives the memory controller a handle on the rank objects
void MemoryController::attachRanks(vector<Rank> *ranks)
{
	this->ranks = ranks;
}

//memory controller update
void MemoryController::update()
{

	//PRINT(" ------------------------- [" << currentClockCycle << "] -------------------------");

	//update bank states
	for (size_t i=0;i<NUM_RANKS;i++)
	{
		for (size_t j=0;j<NUM_BANKS;j++)
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
						bankStates[i][j].stateChangeCountdown = tRP;
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
			(*ranks)[outgoingCmdPacket->rank].receiveFromBus(outgoingCmdPacket);
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
			if (parentMemorySystem->WriteDataDone!=NULL)
			{
				(*parentMemorySystem->WriteDataDone)(parentMemorySystem->systemID,outgoingDataPacket->physicalAddress, currentClockCycle);
			}

			(*ranks)[outgoingDataPacket->rank].receiveFromBus(outgoingDataPacket);
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
			if (DEBUG_BUS)
			{
				PRINTN(" -- MC Issuing On Data Bus    : ");
				writeDataToSend[0]->print();
			}

			// queue up the packet to be sent
			if (outgoingDataPacket != NULL)
			{
				ERROR("== Error - Data Bus Collision");
				exit(-1);
			}

			outgoingDataPacket = writeDataToSend[0];
			dataCyclesLeft = BL/2;

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
		commandQueue.needRefresh(refreshRank);
		(*ranks)[refreshRank].refreshWaiting = true;
		refreshCountdown[refreshRank] =	 REFRESH_PERIOD/tCK;
		refreshRank++;
		if (refreshRank == NUM_RANKS)
		{
			refreshRank = 0;
		}
	}
	//if a rank is powered down, make sure we power it up in time for a refresh
	else if (powerDown[refreshRank] && refreshCountdown[refreshRank] <= tXP)
	{
		(*ranks)[refreshRank].refreshWaiting = true;
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
			writeDataCountdown.push_back(WL);
		}

		//
		//update each bank's state based on the command that was just popped out of the command queue
		//
		//for readability's sake
		uint rank = poppedBusPacket->rank;
		uint bank = poppedBusPacket->bank;
		switch (poppedBusPacket->busPacketType)
		{
		case READ:
			//add energy to account for total
			if (DEBUG_POWER)
			{
				PRINT(" ++ Adding Read energy to total energy");
			}
			burstEnergy[rank] += (IDD4R - IDD3N) * BL/2 * NUM_DEVICES;

			bankStates[rank][bank].nextPrecharge = max(currentClockCycle + READ_TO_PRE_DELAY,
			                                       bankStates[rank][bank].nextPrecharge);
			bankStates[rank][bank].lastCommand = READ;

			for (size_t i=0;i<NUM_RANKS;i++)
			{
				for (size_t j=0;j<NUM_BANKS;j++)
				{
					if (i!=poppedBusPacket->rank)
					{
						//check to make sure it is active before trying to set (save's time?)
						if (bankStates[i][j].currentBankState == RowActive)
						{
							bankStates[i][j].nextRead = max(currentClockCycle + BL/2 + tRTRS, bankStates[i][j].nextRead);
							bankStates[i][j].nextWrite = max(currentClockCycle + READ_TO_WRITE_DELAY,
							                                 bankStates[i][j].nextWrite);
						}
					}
					else
					{
						bankStates[i][j].nextRead = max(currentClockCycle + max(tCCD, BL/2), bankStates[i][j].nextRead);
						bankStates[i][j].nextWrite = max(currentClockCycle + READ_TO_WRITE_DELAY,
						                                 bankStates[i][j].nextWrite);
					}
				}
			}
			break;
		case READ_P:
			//add energy to account for total
			if (DEBUG_POWER)
			{
				PRINT(" ++ Adding Read energy to total energy");
			}
			burstEnergy[rank] += (IDD4R - IDD3N) * BL/2 * NUM_DEVICES;

			//Don't bother setting next read or write times because the bank is no longer active
			//bankStates[rank][bank].currentBankState = Idle;
			bankStates[rank][bank].nextActivate = max(currentClockCycle + READ_AUTOPRE_DELAY,
			                                      bankStates[rank][bank].nextActivate);
			bankStates[rank][bank].lastCommand = READ_P;
			bankStates[rank][bank].stateChangeCountdown = READ_TO_PRE_DELAY;

			for (size_t i=0;i<NUM_RANKS;i++)
			{
				for (size_t j=0;j<NUM_BANKS;j++)
				{
					if (i!=poppedBusPacket->rank)
					{
						//check to make sure it is active before trying to set (save's time?)
						if (bankStates[i][j].currentBankState == RowActive)
						{
							bankStates[i][j].nextRead = max(currentClockCycle + BL/2 + tRTRS, bankStates[i][j].nextRead);
							bankStates[i][j].nextWrite = max(currentClockCycle + READ_TO_WRITE_DELAY,
							                                 bankStates[i][j].nextWrite);
						}
					}
					else
					{
						bankStates[i][j].nextRead = max(currentClockCycle + max(tCCD, BL/2), bankStates[i][j].nextRead);
						bankStates[i][j].nextWrite = max(currentClockCycle + READ_TO_WRITE_DELAY,
						                                 bankStates[i][j].nextWrite);
					}
				}
			}

			//set read and write to nextActivate so the state table will prevent a read or write
			//  being issued (in cq.isIssuable())before the bank state has been changed because of the
			//  auto-precharge associated with this command
			bankStates[rank][bank].nextRead = bankStates[rank][bank].nextActivate;
			bankStates[rank][bank].nextWrite = bankStates[rank][bank].nextActivate;

			break;
		case WRITE:
			//add energy to account for total
			if (DEBUG_POWER)
			{
				PRINT(" ++ Adding Write energy to total energy");
			}
			burstEnergy[rank] += (IDD4W - IDD3N) * BL/2 * NUM_DEVICES;

			bankStates[rank][bank].nextPrecharge = max(currentClockCycle + WRITE_TO_PRE_DELAY,
			                                       bankStates[rank][bank].nextPrecharge);
			bankStates[rank][bank].lastCommand = WRITE;

			for (size_t i=0;i<NUM_RANKS;i++)
			{
				for (size_t j=0;j<NUM_BANKS;j++)
				{
					if (i!=poppedBusPacket->rank)
					{
						if (bankStates[i][j].currentBankState == RowActive)
						{
							bankStates[i][j].nextWrite = max(currentClockCycle + BL/2 + tRTRS, bankStates[i][j].nextWrite);
							bankStates[i][j].nextRead = max(currentClockCycle + WRITE_TO_READ_DELAY_R,
							                                bankStates[i][j].nextRead);
						}
					}
					else
					{
						bankStates[i][j].nextWrite = max(currentClockCycle + max(BL/2, tCCD), bankStates[i][j].nextWrite);
						bankStates[i][j].nextRead = max(currentClockCycle + WRITE_TO_READ_DELAY_B,
						                                bankStates[i][j].nextRead);
					}
				}
			}
			break;
		case WRITE_P:
			//add energy to account for total
			if (DEBUG_POWER)
			{
				PRINT(" ++ Adding Write energy to total energy");
			}
			burstEnergy[rank] += (IDD4W - IDD3N) * BL/2 * NUM_DEVICES;

			bankStates[rank][bank].nextActivate = max(currentClockCycle + WRITE_AUTOPRE_DELAY,
			                                      bankStates[rank][bank].nextActivate);
			bankStates[rank][bank].lastCommand = WRITE_P;
			bankStates[rank][bank].stateChangeCountdown = WRITE_TO_PRE_DELAY;

			for (size_t i=0;i<NUM_RANKS;i++)
			{
				for (size_t j=0;j<NUM_BANKS;j++)
				{
					if (i!=poppedBusPacket->rank)
					{
						if (bankStates[i][j].currentBankState == RowActive)
						{
							bankStates[i][j].nextWrite = max(currentClockCycle + BL/2 + tRTRS, bankStates[i][j].nextWrite);
							bankStates[i][j].nextRead = max(currentClockCycle + WRITE_TO_READ_DELAY_R,
							                                bankStates[i][j].nextRead);
						}
					}
					else
					{
						bankStates[i][j].nextWrite = max(currentClockCycle + max(BL/2, tCCD), bankStates[i][j].nextWrite);
						bankStates[i][j].nextRead = max(currentClockCycle + WRITE_TO_READ_DELAY_B,
						                                bankStates[i][j].nextRead);
					}
				}
			}

			//set read and write to nextActivate so the state table will prevent a read or write
			//  being issued (in cq.isIssuable())before the bank state has been changed because of the
			//  auto-precharge associated with this command
			bankStates[rank][bank].nextRead = bankStates[rank][bank].nextActivate;
			bankStates[rank][bank].nextWrite = bankStates[rank][bank].nextActivate;

			break;
		case ACTIVATE:
			//add energy to account for total
			if (DEBUG_POWER)
			{
				PRINT(" ++ Adding Activate and Precharge energy to total energy");
			}
			actpreEnergy[rank] += ((IDD0 * tRC) - ((IDD3N * tRAS) + (IDD2N * (tRC - tRAS)))) * NUM_DEVICES;

			bankStates[rank][bank].currentBankState = RowActive;
			bankStates[rank][bank].lastCommand = ACTIVATE;
			bankStates[rank][bank].openRowAddress = poppedBusPacket->row;
			bankStates[rank][bank].nextActivate = max(currentClockCycle + tRC, bankStates[rank][bank].nextActivate);
			bankStates[rank][bank].nextPrecharge = max(currentClockCycle + tRAS, bankStates[rank][bank].nextPrecharge);

			//if we are using posted-CAS, the next column access can be sooner than normal operation

			bankStates[rank][bank].nextRead = max(currentClockCycle + (tRCD-AL), bankStates[rank][bank].nextRead);
			bankStates[rank][bank].nextWrite = max(currentClockCycle + (tRCD-AL), bankStates[rank][bank].nextWrite);

			for (size_t i=0;i<NUM_BANKS;i++)
			{
				if (i!=poppedBusPacket->bank)
				{
					bankStates[rank][i].nextActivate = max(currentClockCycle + tRRD, bankStates[rank][i].nextActivate);
				}
			}

			break;
		case PRECHARGE:
			bankStates[rank][bank].currentBankState = Precharging;
			bankStates[rank][bank].lastCommand = PRECHARGE;
			bankStates[rank][bank].stateChangeCountdown = tRP;
			bankStates[rank][bank].nextActivate = max(currentClockCycle + tRP, bankStates[rank][bank].nextActivate);

			break;
		case REFRESH:
			//add energy to account for total
			if (DEBUG_POWER)
			{
				PRINT(" ++ Adding Refresh energy to total energy");
			}
			refreshEnergy[rank] += (IDD5 - IDD3N) * tRFC * NUM_DEVICES;

			for (size_t i=0;i<NUM_BANKS;i++)
			{
				bankStates[rank][i].nextActivate = currentClockCycle + tRFC;
				bankStates[rank][i].currentBankState = Refreshing;
				bankStates[rank][i].lastCommand = REFRESH;
				bankStates[rank][i].stateChangeCountdown = tRFC;
			}

			break;
		default:
			ERROR("== Error - Popped a command we shouldn't have of type : " << poppedBusPacket->busPacketType);
			exit(0);
		}

		//issue on bus and print debug
		if (DEBUG_BUS)
		{
			PRINTN(" -- MC Issuing On Command Bus : ");
			poppedBusPacket->print();
		}

		//check for collision on bus
		if (outgoingCmdPacket != NULL)
		{
			ERROR("== Error - Command Bus Collision");
			exit(-1);
		}
		outgoingCmdPacket = poppedBusPacket;
		cmdCyclesLeft = tCMD;

	}

	for (size_t i=0;i<transactionQueue.size();i++)
	{
		//pop off top transaction from queue
		//
		//	assuming simple scheduling at the moment
		//	will eventually add policies here
		Transaction transaction = transactionQueue[i];

		//map address to rank,bank,row,col
		uint newTransactionRank, newTransactionBank, newTransactionRow, newTransactionColumn;

		// pass these in as references so they get set by the addressMapping function
		addressMapping(transaction.address, newTransactionRank, newTransactionBank, newTransactionRow, newTransactionColumn);

		//if we have room, break up the transaction into the appropriate commands
		//and add them to the command queue
		if (commandQueue.hasRoomFor(2, newTransactionRank, newTransactionBank))
		{

			if (DEBUG_ADDR_MAP) 
			{
				PRINTN("== New Transaction - Mapping Address [0x" << hex << transaction.address << dec << "]");
				if (transaction.transactionType == DATA_READ) 
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

			if (transaction.transactionType == DATA_READ)
			{
				pendingReadTransactions.push_back(transaction);
			}
			//now that we know there is room, we can remove from the transaction queue
			transactionQueue.erase(transactionQueue.begin()+i);

			//create activate command to the row we just translated
			BusPacket *ACTcommand = new BusPacket(ACTIVATE, transaction.address, newTransactionColumn, newTransactionRow,
			                                 newTransactionRank, newTransactionBank, 0);
			commandQueue.enqueue(ACTcommand);

			//create read or write command and enqueue it
			if (transaction.transactionType == DATA_READ)
			{
				BusPacket *READcommand;
				if (rowBufferPolicy == OpenPage)
				{
					READcommand = new BusPacket(READ, transaction.address, newTransactionColumn, newTransactionRow,
					                        newTransactionRank, newTransactionBank,0);
					commandQueue.enqueue(READcommand);
				}
				else if (rowBufferPolicy == ClosePage)
				{
					READcommand = new BusPacket(READ_P, transaction.address, newTransactionColumn, newTransactionRow,
					                        newTransactionRank, newTransactionBank,0);
					commandQueue.enqueue(READcommand);
				}
			}
			else if (transaction.transactionType == DATA_WRITE)
			{
				BusPacket *WRITEcommand;
				if (rowBufferPolicy == OpenPage)
				{
					WRITEcommand = new BusPacket(WRITE, transaction.address, newTransactionColumn, newTransactionRow,
					                         newTransactionRank, newTransactionBank, transaction.data);
					commandQueue.enqueue(WRITEcommand);
				}
				else if (rowBufferPolicy == ClosePage)
				{
					WRITEcommand = new BusPacket(WRITE_P, transaction.address, newTransactionColumn, newTransactionRow,
					                         newTransactionRank, newTransactionBank, transaction.data);
					commandQueue.enqueue(WRITEcommand);
				}
			}
			else
			{
				ERROR("== Error -	 Unknown transaction type");
				exit(0);
			}
		}
		else
		{
			//PRINT( "== Warning - No room in command queue" << endl;
		}
	}


	//calculate power
	//  this is done on a per-rank basis, since power characterization is done per device (not per bank)
	for (size_t i=0;i<NUM_RANKS;i++)
	{
		if (USE_LOW_POWER)
		{
			//if there are no commands in the queue and that particular rank is not waiting for a refresh...
			if (commandQueue.isEmpty(i) && !(*ranks)[i].refreshWaiting)
			{
				//check to make sure all banks are idle
				bool allIdle = true;
				for (size_t j=0;j<NUM_BANKS;j++)
				{
					if (bankStates[i][j].currentBankState != Idle)
					{
						allIdle = false;
					}
				}

				//if they ARE all idle, put in power down mode and set appropriate fields
				if (allIdle)
				{
					powerDown[i] = true;
					(*ranks)[i].powerDown();
					for (size_t j=0;j<NUM_BANKS;j++)
					{
						bankStates[i][j].currentBankState = PowerDown;
						bankStates[i][j].nextPowerUp = currentClockCycle + tCKE;
					}
				}
			}
			//if there IS something in the queue or there IS a refresh waiting (and we can power up), do it
			else if (currentClockCycle >= bankStates[i][0].nextPowerUp && powerDown[i]) //use 0 since theyre all the same
			{
				powerDown[i] = false;
				(*ranks)[i].powerUp();
				for (size_t j=0;j<NUM_BANKS;j++)
				{
					bankStates[i][j].currentBankState = Idle;
					bankStates[i][j].nextActivate = currentClockCycle + tXP;
				}
			}
		}

		//check for open bank
		bool bankOpen = false;
		for (size_t j=0;j<NUM_BANKS;j++)
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
			if (DEBUG_POWER)
			{
				PRINT(" ++ Adding IDD3N to total energy [from rank "<< i <<"]");
			}
			backgroundEnergy[i] += IDD3N * NUM_DEVICES;
		}
		else
		{
			//if we're in power-down mode, use the correct current
			if (powerDown[i])
			{
				if (DEBUG_POWER)
				{
					PRINT(" ++ Adding IDD2P to total energy [from rank " << i << "]");
				}
				backgroundEnergy[i] += IDD2P * NUM_DEVICES;
			}
			else
			{
				if (DEBUG_POWER)
				{
					PRINT(" ++ Adding IDD2N to total energy [from rank " << i << "]");
				}
				backgroundEnergy[i] += IDD2N * NUM_DEVICES;
			}
		}
	}

	//check for outstanding data to return to the CPU
	if (returnTransaction.size()>0)
	{
		if (DEBUG_BUS)
		{
			PRINTN(" -- MC Issuing to CPU bus : ");
			returnTransaction[0].print();
		}
		totalTransactions++;

		//find the pending read transaction to calculate latency
		for (size_t i=0;i<pendingReadTransactions.size();i++)
		{
			if (pendingReadTransactions[i].address == returnTransaction[0].address)
			{
				//if(currentClockCycle - pendingReadTransactions[i].timeAdded > 2000)
				//	{
				//		pendingReadTransactions[i].print();
				//		exit(0);
				//	}
				uint rank,bank,row,col;
				addressMapping(returnTransaction[0].address,rank,bank,row,col);
				insertHistogram(currentClockCycle-pendingReadTransactions[i].timeAdded,rank,bank);
				//return latency
				returnReadData(pendingReadTransactions[i]);

				pendingReadTransactions.erase(pendingReadTransactions.begin()+i);
				break;
			}
		}
		returnTransaction.erase(returnTransaction.begin());
	}

	//decrement refresh counters
	for (size_t i=0;i<NUM_RANKS;i++)
	{
		refreshCountdown[i]--;
	}

	//
	//print debug
	//
	if (DEBUG_TRANS_Q)
	{
		PRINT("== Printing transaction queue");
		for (size_t i=0;i<transactionQueue.size();i++)
		{
			PRINTN("  " << i << "]");
			transactionQueue[i].print();
		}
	}

	if (DEBUG_BANKSTATE)
	{
		//TODO: move this to BankState.cpp
		PRINT("== Printing bank states (According to MC)");
		for (size_t i=0;i<NUM_RANKS;i++)
		{
			for (size_t j=0;j<NUM_BANKS;j++)
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

	if (DEBUG_CMD_Q)
	{
		commandQueue.print();
	}

	commandQueue.step();

	//print stats if we're at the end of an epoch
	if (currentClockCycle % EPOCH_COUNT == 0)
	{
		this->printStats();

		totalTransactions = 0;
		for (size_t i=0;i<NUM_RANKS;i++)
		{
			for (size_t j=0; j<NUM_BANKS; j++)
			{
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
}

bool MemoryController::WillAcceptTransaction()
{
	return transactionQueue.size() < TRANS_QUEUE_DEPTH;
}

//allows outside source to make request of memory system
bool MemoryController::addTransaction(Transaction &trans)
{
	if (WillAcceptTransaction())
	{
		trans.timeAdded = currentClockCycle;
		transactionQueue.push_back(trans);
		return true;
	}
	else 
	{
		return false;
	}
}

//Breaks up the incoming transaction into commands
void MemoryController::addressMapping(uint64_t physicalAddress, uint &newTransactionRank, uint &newTransactionBank, uint &newTransactionRow, uint &newTransactionColumn)
{
	uint64_t tempA, tempB;


	physicalAddress = physicalAddress >> byteOffsetWidth;

	//perform various address mapping schemes
	if (addressMappingScheme == Scheme1)
	{
		//chan:rank:row:col:bank
		tempA = physicalAddress;
		physicalAddress = physicalAddress >> bankBitWidth;
		tempB = physicalAddress << bankBitWidth;
		newTransactionBank = tempA ^ tempB;

		tempA = physicalAddress;
		physicalAddress = physicalAddress >> colBitWidth;
		tempB = physicalAddress << colBitWidth;
		newTransactionColumn = tempA ^ tempB;

		tempA = physicalAddress;
		physicalAddress = physicalAddress >> rowBitWidth;
		tempB = physicalAddress << rowBitWidth;
		newTransactionRow = tempA ^ tempB;

		tempA = physicalAddress;
		physicalAddress = physicalAddress >> rankBitWidth;
		tempB = physicalAddress << rankBitWidth;
		newTransactionRank = tempA ^ tempB;
	}
	else if (addressMappingScheme == Scheme2)
	{
		//chan:row:col:bank:rank
		tempA = physicalAddress;
		physicalAddress = physicalAddress >> rankBitWidth;
		tempB = physicalAddress << rankBitWidth;
		newTransactionRank = tempA ^ tempB;

		tempA = physicalAddress;
		physicalAddress = physicalAddress >> bankBitWidth;
		tempB = physicalAddress << bankBitWidth;
		newTransactionBank = tempA ^ tempB;

		tempA = physicalAddress;
		physicalAddress = physicalAddress >> colBitWidth;
		tempB = physicalAddress << colBitWidth;
		newTransactionColumn = tempA ^ tempB;

		tempA = physicalAddress;
		physicalAddress = physicalAddress >> rowBitWidth;
		tempB = physicalAddress << rowBitWidth;
		newTransactionRow = tempA ^ tempB;
	}
	else if (addressMappingScheme == Scheme3)
	{
		//chan:rank:bank:col:row
		tempA = physicalAddress;
		physicalAddress = physicalAddress >> rowBitWidth;
		tempB = physicalAddress << rowBitWidth;
		newTransactionRow = tempA ^ tempB;

		tempA = physicalAddress;
		physicalAddress = physicalAddress >> colBitWidth;
		tempB = physicalAddress << colBitWidth;
		newTransactionColumn = tempA ^ tempB;

		tempA = physicalAddress;
		physicalAddress = physicalAddress >> bankBitWidth;
		tempB = physicalAddress << bankBitWidth;
		newTransactionBank = tempA ^ tempB;

		tempA = physicalAddress;
		physicalAddress = physicalAddress >> rankBitWidth;
		tempB = physicalAddress << rankBitWidth;
		newTransactionRank = tempA ^ tempB;
	}
	else if (addressMappingScheme == Scheme4)
	{
		//chan:rank:bank:row:col
		tempA = physicalAddress;
		physicalAddress = physicalAddress >> colBitWidth;
		tempB = physicalAddress << colBitWidth;
		newTransactionColumn = tempA ^ tempB;

		tempA = physicalAddress;
		physicalAddress = physicalAddress >> rowBitWidth;
		tempB = physicalAddress << rowBitWidth;
		newTransactionRow = tempA ^ tempB;

		tempA = physicalAddress;
		physicalAddress = physicalAddress >> bankBitWidth;
		tempB = physicalAddress << bankBitWidth;
		newTransactionBank = tempA ^ tempB;

		tempA = physicalAddress;
		physicalAddress = physicalAddress >> rankBitWidth;
		tempB = physicalAddress << rankBitWidth;
		newTransactionRank = tempA ^ tempB;
	}
	else if (addressMappingScheme == Scheme5)
	{
		//chan:row:col:rank:bank

		tempA = physicalAddress;
		physicalAddress = physicalAddress >> bankBitWidth;
		tempB = physicalAddress << bankBitWidth;
		newTransactionBank = tempA ^ tempB;

		tempA = physicalAddress;
		physicalAddress = physicalAddress >> rankBitWidth;
		tempB = physicalAddress << rankBitWidth;
		newTransactionRank = tempA ^ tempB;

		tempA = physicalAddress;
		physicalAddress = physicalAddress >> colBitWidth;
		tempB = physicalAddress << colBitWidth;
		newTransactionColumn = tempA ^ tempB;

		tempA = physicalAddress;
		physicalAddress = physicalAddress >> rowBitWidth;
		tempB = physicalAddress << rowBitWidth;
		newTransactionRow = tempA ^ tempB;

	}
	else if (addressMappingScheme == Scheme6)
	{
		//chan:row:bank:rank:col

		tempA = physicalAddress;
		physicalAddress = physicalAddress >> colBitWidth;
		tempB = physicalAddress << colBitWidth;
		newTransactionColumn = tempA ^ tempB;

		tempA = physicalAddress;
		physicalAddress = physicalAddress >> rankBitWidth;
		tempB = physicalAddress << rankBitWidth;
		newTransactionRank = tempA ^ tempB;

		tempA = physicalAddress;
		physicalAddress = physicalAddress >> bankBitWidth;
		tempB = physicalAddress << bankBitWidth;
		newTransactionBank = tempA ^ tempB;

		tempA = physicalAddress;
		physicalAddress = physicalAddress >> rowBitWidth;
		tempB = physicalAddress << rowBitWidth;
		newTransactionRow = tempA ^ tempB;

	}
	else
	{
		ERROR("== Error - Unknown Address Mapping Scheme");
		exit(-1);
	}

}

//prints statistics at the end of an epoch or  simulation
void MemoryController::printStats(bool finalStats)
{
	//skip the print on the first cycle, it's pretty useless
	if (currentClockCycle == 0)
		return;

//TODO: move Vdd to config file
	float Vdd = 1.9;

	//if we are not at the end of the epoch, make sure to adjust for the actual number of cycles elapsed

	uint64_t cyclesElapsed = (currentClockCycle % EPOCH_COUNT == 0) ? EPOCH_COUNT : currentClockCycle % EPOCH_COUNT;
	uint bytesPerTransaction = (64*BL)/8;
	uint64_t totalBytesTransferred = totalTransactions * bytesPerTransaction;
	double secondsThisEpoch = (double)cyclesElapsed * tCK * 1E-9;

	// only per rank
	vector<double> backgroundPower = vector<double>(NUM_RANKS,0.0);
	vector<double> burstPower = vector<double>(NUM_RANKS,0.0);
	vector<double> refreshPower = vector<double>(NUM_RANKS,0.0);
	vector<double> actprePower = vector<double>(NUM_RANKS,0.0);
	vector<double> averagePower = vector<double>(NUM_RANKS,0.0);

	// per bank variables
	vector<double> averageLatency = vector<double>(NUM_RANKS*NUM_BANKS,0.0);
	vector<double> bandwidth = vector<double>(NUM_RANKS*NUM_BANKS,0.0);

	double totalBandwidth=0.0;
	for (size_t i=0;i<NUM_RANKS;i++)
	{
		for (size_t j=0; j<NUM_BANKS; j++)
		{
			bandwidth[SEQUENTIAL(i,j)] = (((double)(totalReadsPerBank[SEQUENTIAL(i,j)]+totalWritesPerBank[SEQUENTIAL(i,j)]) * (double)bytesPerTransaction)/(1024.0*1024.0*1024.0)) / secondsThisEpoch;
			averageLatency[SEQUENTIAL(i,j)] = ((float)totalEpochLatency[SEQUENTIAL(i,j)] / (float)(totalReadsPerBank[SEQUENTIAL(i,j)])) * tCK;
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


	(*visDataOut) << currentClockCycle * tCK * 1E-6<< ":";

	for (size_t i=0;i<NUM_RANKS;i++)
	{

		PRINT( "      -Rank   "<<i<<" : ");
		PRINTN( "        -Reads  : " << totalReadsPerRank[i]);
		PRINT( " ("<<totalReadsPerRank[i] * bytesPerTransaction<<" bytes)");
		PRINTN( "        -Writes : " << totalWritesPerRank[i]);
		PRINT( " ("<<totalWritesPerRank[i] * bytesPerTransaction<<" bytes)");
		for (size_t j=0;j<NUM_BANKS;j++)
		{
			PRINT( "        -Bandwidth / Latency  (Bank " <<j<<"): " <<bandwidth[SEQUENTIAL(i,j)] << " GB/s\t\t" <<averageLatency[SEQUENTIAL(i,j)] << " ns");
		}

		// factor of 1000 at the end is to account for the fact that totalEnergy is accumulated in mJ since IDD values are given in mA
		backgroundPower[i] = ((double)backgroundEnergy[i] / (double)(cyclesElapsed)) * Vdd / 1000.0;
		burstPower[i] = ((double)burstEnergy[i] / (double)(cyclesElapsed)) * Vdd / 1000.0;
		refreshPower[i] = ((double) refreshEnergy[i] / (double)(cyclesElapsed)) * Vdd / 1000.0;
		actprePower[i] = ((double)actpreEnergy[i] / (double)(cyclesElapsed)) * Vdd / 1000.0;
		averagePower[i] = ((backgroundEnergy[i] + burstEnergy[i] + refreshEnergy[i] + actpreEnergy[i]) / (double)cyclesElapsed) * Vdd / 1000.0;

		if ((*parentMemorySystem->ReportPower)!=NULL)
		{
			(*parentMemorySystem->ReportPower)(backgroundPower[i],burstPower[i],refreshPower[i],actprePower[i]);
		}

		PRINT( " == Power Data for Rank        " << i );
		PRINT( "   Average Power (watts)     : " << averagePower[i] );
		PRINT( "     -Background (watts)     : " << backgroundPower[i] );
		PRINT( "     -Act/Pre    (watts)     : " << actprePower[i] );
		PRINT( "     -Burst      (watts)     : " << burstPower[i]);
		PRINT( "     -Refresh    (watts)     : " << refreshPower[i] );

		// write the vis file output
		(*visDataOut) << "bgp_"<<i<<"="<<backgroundPower[i]<<",";
		(*visDataOut) << "ap_"<<i<<"="<<actprePower[i]<<",";
		(*visDataOut) << "bp_"<<i<<"="<<burstPower[i]<<",";
		(*visDataOut) << "rp_"<<i<<"="<<refreshPower[i]<<",";
		for (size_t j=0; j<NUM_BANKS; j++)
		{
			(*visDataOut) << "b_" <<i<<"_"<<j<<"="<<bandwidth[SEQUENTIAL(i,j)]<<",";
			(*visDataOut) << "l_" <<i<<"_"<<j<<"="<<averageLatency[SEQUENTIAL(i,j)]<<",";
		}
	}

	(*visDataOut) <<endl;

	// only print the latency histogram at the end of the simulation since it clogs the output too much to print every epoch
	if (finalStats)
	{
		PRINT( " ---  Latency list ("<<latencies.size()<<")");
		PRINT( "       [lat] : #");

		(*visDataOut) << "!!HISTOGRAM_DATA"<<endl;

		map<uint,uint>::iterator it; //
		for (it=latencies.begin(); it!=latencies.end(); it++)
		{
			PRINT( "       ["<< it->first <<"-"<<it->first+(HISTOGRAM_BIN_SIZE-1)<<"] : "<< it->second );
			(*visDataOut) << it->first <<"="<< it->second << endl;
		}

		PRINT( " ---  Bank usage list");
		for (size_t i=0;i<NUM_RANKS;i++)
		{
			for (size_t j=0;j<NUM_BANKS;j++)
			{
				PRINT( "["<<i<<","<<j<<"] : "<<totalReadsPerBank[i+j]+totalWritesPerBank[i+j]);
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
}
MemoryController::~MemoryController()
{
	//ERROR("MEMORY CONTROLLER DESTRUCTOR");
	//abort();
}
//inserts a latency into the latency histogram
void MemoryController::insertHistogram(uint latencyValue, uint rank, uint bank)
{
	totalEpochLatency[SEQUENTIAL(rank,bank)] += latencyValue;
	//poor man's way to bin things.
	latencies[(latencyValue/HISTOGRAM_BIN_SIZE)*HISTOGRAM_BIN_SIZE]++;
}
