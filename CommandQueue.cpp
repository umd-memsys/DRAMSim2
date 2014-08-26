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


//CommandQueue.cpp
//
//Class file for command queue object
//

#include "ConfigIniReader.h"
#include "CommandQueue.h"
#include "MemoryController.h"
#include "PrintMacros.h"
#include <assert.h>
#include <set>

using namespace DRAMSim;

CommandQueue::CommandQueue(vector< vector<BankState> > &states, ostream &dramsim_log_, const Config &cfg_) :
		dramsim_log(dramsim_log_),
		cfg(cfg_),
		bankStates(states),
		nextBank(0),
		nextRank(0),
		nextBankPRE(0),
		nextRankPRE(0),
		refreshRank(0),
		refreshWaiting(false),
		sendAct(true)
{
	//set here to avoid compile errors
	currentClockCycle = 0;

	//use numBankQueus below to create queue structure
	size_t numBankQueues;
	if (cfg.QUEUING_STRUCTURE==PerRank)
	{
		numBankQueues = 1;
	}
	else if (cfg.QUEUING_STRUCTURE==PerRankPerBank)
	{
		numBankQueues = cfg.NUM_BANKS;
	}
	else
	{
		ERROR("== Error - Unknown queuing structure");
		exit(0);
	}

	//vector of counters used to ensure rows don't stay open too long
	rowAccessCounters = vector< vector<unsigned> >(cfg.NUM_RANKS, vector<unsigned>(cfg.NUM_BANKS,0));

	//create queue based on the structure we want
	BusPacket1D actualQueue;
	BusPacket2D perBankQueue = BusPacket2D();
	queues = BusPacket3D();
	for (size_t rank=0; rank<cfg.NUM_RANKS; rank++)
	{
		//this loop will run only once for per-rank and cfg.NUM_BANKS times for per-rank-per-bank
		for (size_t bank=0; bank<numBankQueues; bank++)
		{
			actualQueue	= BusPacket1D();
			perBankQueue.push_back(actualQueue);
		}
		queues.push_back(perBankQueue);
	}


	//FOUR-bank activation window
	//	this will count the number of activations within a given window
	//	(decrementing counter)
	//
	//countdown vector will have decrementing counters starting at tFAW
	//  when the 0th element reaches 0, remove it
	for (size_t i=0;i<cfg.NUM_RANKS;i++)
	{
		//init the empty vectors here so we don't seg fault later
		tFAWCountdown.push_back(vector<unsigned>());
	}
}
CommandQueue::~CommandQueue()
{
	//ERROR("COMMAND QUEUE destructor");
	size_t bankMax = cfg.NUM_RANKS;
	if (cfg.QUEUING_STRUCTURE == PerRank) {
		bankMax = 1; 
	}
	// TODO: and ....? 
}
//Adds a command to appropriate queue
void CommandQueue::enqueue(BusPacket *newBusPacket)
{
	unsigned rank = newBusPacket->rank;
	unsigned bank = newBusPacket->bank;

	vector<BusPacket *> &queue = getCommandQueue(rank, bank);
	assert(queue.size() < cfg.CMD_QUEUE_DEPTH && "Queue Overrun, did you call hasRoomFor()?");

	// Scan the queue and add any dependencies for this packet 
	for (size_t i=0, end = queue.size(); i<end; ++i) {
		if (newBusPacket->isDependent(queue[i])) {
			newBusPacket->setDependsOn(queue[i]);
		}
	}
	queue.push_back(newBusPacket);
}
void CommandQueue::injectPrecharges(unsigned rank, unsigned bank) 
{
	if (cfg.ROW_BUFFER_POLICY != OpenPage) 
	{
		return;
	}
	vector<BusPacket *> &queue = getCommandQueue(rank, bank);
	for (int i=queue.size()-1; i >= 0; i--) {

	}

}
void CommandQueue::injectRefreshCommand(unsigned refreshRank) 
{
	// just cover the PerRank case here for now 
	assert(cfg.QUEUING_STRUCTURE != PerRankPerBank && "PerRankPerBank is deprecated");
	vector<BusPacket *> &queue = getCommandQueue(refreshRank, 0); 

	// in all cases, if a refresh is waiting, we need to stop all new
	// activations; what we do is we create a refresh packet, and make all
	// activates in the queue depend on the refresh packet
	BusPacket *refreshPacket = BusPacketPool.alloc();
	refreshPacket->init(REFRESH, 0, 0, 0, refreshRank, 0, cfg, NULL);

	// keep track of banks that we've added precharges for in open page mode
	std::set<unsigned> banksPrecharged; 

	for (unsigned i=0, end = queue.size(); i<end; ++i) 
	{
		BusPacket *bp = queue[i];
		if (bp->busPacketType == ACTIVATE) 
		{
			bp->setDependsOn(refreshPacket);
		} 
		else // not an activate
		{
			// all column access commands in open page mode
			if (cfg.ROW_BUFFER_POLICY == OpenPage && bp->busPacketType != PRECHARGE)
			{
				// now, in open page mode, we also need to add some precharges to the queue
				// to close out all the banks and make our refresh depend on all those
				// precharges 
				BankState &state = bankStates[refreshRank][bp->bank];
				if (state.currentBankState == RowActive &&
						banksPrecharged.count(bp->bank) == 0 ) 
				{
					BusPacket *prechargePacket = BusPacketPool.alloc(); 
					prechargePacket->init(PRECHARGE, 0, 0, 0, refreshRank, bp->bank, cfg, NULL);
					// only need to send one precharge per bank
					if (banksPrecharged.count(bp->bank) == 0) 
					{
						// walk to the end of the queue and add dependencies to this
						// precharge packet
						for (unsigned j=i, end = queue.size(); j<end; j++) {
							if (queue[j]->bank == bp->bank) 
							{
								prechargePacket->setDependsOn(queue[j]);
							}
						}
						// finally, set the refresh packet to depend on the precharge
						refreshPacket->setDependsOn(prechargePacket);
						banksPrecharged.insert(bp->bank);
						queue.push_back(prechargePacket);
					}
				}
			}
			queue.push_back(refreshPacket);
		}
	}
}

// FIXME: hack -- need to rewrite pop() to be better
BusPacket *CommandQueue::pop() {
	BusPacket *bp = actual_pop();
	//if its an activate, add a tfaw counter
	if (bp && bp->busPacketType == ACTIVATE) {
		tFAWCountdown[bp->rank].push_back(cfg.tFAW);
	}
	return bp;
}

//Removes the next item from the command queue based on the system's
//command scheduling policy
BusPacket *CommandQueue::actual_pop()
{
	BusPacket *returnPacket = NULL;
	//this can be done here because pop() is called every clock cycle by the parent MemoryController
	//	figures out the sliding window requirement for tFAW
	//
	//deal with tFAW book-keeping
	//	each rank has it's own counter since the restriction is on a device level
	for (size_t i=0;i<cfg.NUM_RANKS;i++)
	{
		//decrement all the counters we have going
		for (size_t j=0, end=tFAWCountdown[i].size(); j < end ;j++)
		{
			tFAWCountdown[i][j]--;
		}

		//the head will always be the smallest counter, so check if it has reached 0
		if (!tFAWCountdown[i].empty() && tFAWCountdown[i][0]==0)
		{
			tFAWCountdown[i].erase(tFAWCountdown[i].begin());
		}
	}

	/* Now we need to find a packet to issue. 
		 
		 First the code looks if any refreshes need to go
		 Then it looks for data packets
		 Otherwise, it starts looking for rows to close (in open page)
	*/

	// this is now independent of the row buffer policy
	if (refreshWaiting)
	{
		// only examines refreshRank, but if we find something to send, that's
		// it for this function 
		BusPacket *bp = sendRefreshOrCloseOpenBank();
		if (bp) 
			return bp;
	} 

	// TODO: these global round-robin bank loops get tedious -- in this case,
	// can just make a RoundRobinContainer of 
	if (cfg.ROW_BUFFER_POLICY==ClosePage)
	{

		bool foundIssuable = false;
		// used to check loop end condition 
		unsigned startingRank = nextRank;
		unsigned startingBank = nextBank;
		// modulo loop over ranks
		do
		{
			vector<BusPacket *> &queue = getCommandQueue(nextRank, nextBank);
			//make sure there is something in this queue first
			//	also make sure a rank isn't waiting for a refresh
			//	if a rank is waiting for a refesh, don't issue anything to it until the
			//		refresh logic above has sent one out (ie, letting banks close)
			if (!queue.empty() && !((nextRank == refreshRank) && refreshWaiting))
			{
				if (cfg.QUEUING_STRUCTURE == PerRank)
				{
					//search from beginning to find first issuable bus packet
					for (size_t i=0;i<queue.size();i++)
					{
						if (isIssuable(queue[i]))
						{
							returnPacket = queue[i];
							queue.erase(queue.begin()+i);
							return returnPacket;
						}
						if (cfg.QUEUING_STRUCTURE == PerRankPerBank) {
							//no need to search because if the front can't be sent,
							// then no chance something behind it can go instead
							break;
						}
					}
				}
			}

			//rank round robin
			if (cfg.QUEUING_STRUCTURE == PerRank)
			{
				nextRank = (nextRank + 1) % cfg.NUM_RANKS;
				if (startingRank == nextRank)
				{
					break;
				}
			}
			else 
			{
				nextRankAndBank(nextRank, nextBank);
				if (startingRank == nextRank && startingBank == nextBank)
				{
					break;
				}
			}
		}
		while (true);

		//if we couldn't find anything to send
		if (!foundIssuable) return NULL;
	}
	else if (cfg.ROW_BUFFER_POLICY==OpenPage)
	{
		unsigned startingRank = nextRank;
		unsigned startingBank = nextBank;
		bool foundIssuable = false;
		do // round robin over queues
		{
			vector<BusPacket *> &queue = getCommandQueue(nextRank,nextBank);
			//make sure there is something there first
			if (!queue.empty() && !((nextRank == refreshRank) && refreshWaiting))
			{
				//search from the beginning to find first issuable bus packet
				for (size_t i=0;i<queue.size();i++)
				{
					BusPacket *packet = queue[i];
					if (isIssuable(packet) && !packet->hasDependencies())
					{
						//if the bus packet before is an activate, that is the act that was
						//	paired with the column access we are removing, so we have to remove
						//	that activate as well (check i>0 because if i==0 then theres nothing before it)
						if (i>0 && queue[i-1]->busPacketType == ACTIVATE)
						{
							rowAccessCounters[packet->rank][packet->bank]++;
							// i is being returned, but i-1 is being thrown away, so must delete it here 
							BusPacketPool.dealloc(queue[i-1]);

							// remove both i-1 (the activate) and i (we've saved the pointer in *busPacket)
							queue.erase(queue.begin()+i-1,queue.begin()+i+1);
						}
						else // there's no activate before this packet
						{
							//or just remove the one bus packet
							queue.erase(queue.begin()+i);
						}

						foundIssuable = true;
						break;
					}
				}
			}

			//if we found something, break out of do-while
			if (foundIssuable) break;

			//rank round robin
			if (cfg.QUEUING_STRUCTURE == PerRank)
			{
				nextRank = (nextRank + 1) % cfg.NUM_RANKS;
				if (startingRank == nextRank)
				{
					break;
				}
			}
			else 
			{
				nextRankAndBank(nextRank, nextBank); 
				if (startingRank == nextRank && startingBank == nextBank)
				{
					break;
				}
			}
		}
		while (true);

		//if nothing was issuable, see if we can issue a PRE to an open bank
		//	that has no other commands waiting
		if (!foundIssuable)
		{
			//search for banks to close
			bool sendingPRE = false;
			unsigned startingRank = nextRankPRE;
			unsigned startingBank = nextBankPRE;

			do // round robin over all ranks and banks
			{
				BankState &bankState = bankStates[nextRankPRE][nextBankPRE];
				vector <BusPacket *> &queue = getCommandQueue(nextRankPRE, nextBankPRE);
				bool found = false;
				//check if bank is open
				if (bankState.currentBankState == RowActive)
				{
					for (size_t i=0;i<queue.size();i++)
					{
						//if there is something going to that bank and row, then we don't want to send a PRE
						if (queue[i]->bank == nextBankPRE &&
								queue[i]->row == bankState.openRowAddress)
						{
							found = true;
							break;
						}
					}

					//if nothing found going to that bank and row or too many accesses have happend, close it
					if (!found || rowAccessCounters[nextRankPRE][nextBankPRE]==cfg.TOTAL_ROW_ACCESSES)
					{
						if (currentClockCycle >= bankState.nextPrecharge)
						{
							sendingPRE = true;
							rowAccessCounters[nextRankPRE][nextBankPRE] = 0;
							BusPacket *busPacket = BusPacketPool.alloc(); 
							busPacket->init(PRECHARGE, 0, 0, 0, nextRankPRE, nextBankPRE, cfg, NULL);
							return busPacket;
						}
					}
				}
				nextRankAndBank(nextRankPRE, nextBankPRE);
			}
			while (!(startingRank == nextRankPRE && startingBank == nextBankPRE));

			//if no PREs could be sent, just return false
			if (!sendingPRE) return NULL;
		}
	}

	//sendAct is flag used for posted-cas
	//  posted-cas is enabled when AL>0
	//  when sendAct is true, when don't want to increment our indexes
	//  so we send the column access that is paid with this act
	if (cfg.AL>0 && sendAct)
	{
		sendAct = false;
	}
	else
	{
		sendAct = true;
		nextRankAndBank(nextRank, nextBank);
	}

	return NULL; 
}

/**
 * In preparation for a refresh, we must go through and close all banks and
 * then send a refresh
 *
 * This function will return either a command or a refresh packet to issue
 * or NULL if nothing is issuable (due to a timing constraint)
 */
BusPacket *CommandQueue::sendRefreshOrCloseOpenBank()
{
	for (size_t b=0;b<cfg.NUM_BANKS;b++)
	{
		vector<BusPacket *> &refreshQueue = getCommandQueue(refreshRank,b);
		if (bankStates[refreshRank][b].currentBankState == RowActive)
		{
			//if the bank is open, make sure there is nothing else
			// going there before we close it
			for (size_t j=0;j<refreshQueue.size();j++)
			{
				BusPacket *packet = refreshQueue[j];
				if (packet->row == bankStates[refreshRank][b].openRowAddress &&
						packet->bank == b)
				{
					// if the row is already open and we have a packet
					// (READ_P/WRITE_P or PRE) going to it, we need to send that first so the
					// bank is closed before the refresh
					if (packet->busPacketType != ACTIVATE && isIssuable(packet))
					{
						refreshQueue.erase(refreshQueue.begin() + j);
						return packet;
					}
				}
			}
			// no requests to open row, precharge the row
			if (currentClockCycle >= bankStates[refreshRank][b].nextPrecharge) {
				rowAccessCounters[refreshRank][b]=0;
				BusPacket *bp = BusPacketPool.alloc(); 
				bp->init(PRECHARGE, 0, 0, 0, refreshRank, b, cfg, NULL);
				return bp;
			}
		} // the bank is closed; check timing to see if we can send REF

		//	NOTE: checks nextActivate time for each bank to make sure tRP is being
		//				satisfied.	the next ACT and next REF can be issued at the same
		//				point in the future, so just use nextActivate field instead of
		//				creating a nextRefresh field
		else if (bankStates[refreshRank][b].nextActivate <= currentClockCycle &&
				bankStates[refreshRank][b].currentBankState != PowerDown)
		{
				// reset refresh counters

				BusPacket *bp = BusPacketPool.alloc(); 
				bp->init(REFRESH, 0, 0, 0, refreshRank, b, cfg, NULL);

				this->refreshRank = -1;
				this->refreshWaiting = false;

				return bp;
		}
	}
	// cannot do anything due to timing constraints or power down 
	return NULL;
}

//check if a rank/bank queue has room for a certain number of bus packets
bool CommandQueue::hasRoomFor(unsigned numberToEnqueue, unsigned rank, unsigned bank)
{
	const vector<BusPacket *> &queue = getCommandQueue(rank, bank); 
	return (cfg.CMD_QUEUE_DEPTH - queue.size() >= numberToEnqueue);
}

//prints the contents of the command queue
void CommandQueue::print() const
{
	PRINT("\n== Printing Per Rank Queue" );
	for (size_t i=0;i<cfg.NUM_RANKS;i++)
	{
		PRINT(" = Rank " << i << "  size : " << queues[i][0].size() );
		for (size_t j=0;j<queues[i][0].size();j++)
		{
			PRINTN("    "<< j << "]" << *queues[i][0][j]);
		}
	}
}

/** 
 * return a reference to the queue for a given rank, bank. Since we
 * don't always have a per bank queuing structure, sometimes the bank
 * argument is ignored (and the 0th index is returned 
 */
vector<BusPacket *> &CommandQueue::getCommandQueue(unsigned rank, unsigned bank)
{
	if (cfg.QUEUING_STRUCTURE == PerRankPerBank)
	{
		return queues[rank][bank];
	}
	else if (cfg.QUEUING_STRUCTURE == PerRank)
	{
		return queues[rank][0];
	}
	else
	{
		ERROR("Unknown queue structure");
		abort(); 
	}

}

//checks if busPacket is allowed to be issued
bool CommandQueue::isIssuable(const BusPacket *busPacket) const
{
	unsigned rank = busPacket->rank;
	unsigned bank = busPacket->bank;
	BankState &bankState = bankStates[rank][bank];

	switch (busPacket->busPacketType)
	{
	case REFRESH:
		
		break;
	case ACTIVATE:
		return ((bankState.currentBankState == Idle ||
		        bankState.currentBankState == Refreshing) &&
		        currentClockCycle >= bankState.nextActivate &&
		        tFAWCountdown[rank].size() < 4);
	case WRITE:
	case WRITE_P:
		return (bankState.currentBankState == RowActive &&
		        currentClockCycle >= bankState.nextWrite &&
		        busPacket->row == bankState.openRowAddress &&
		        rowAccessCounters[rank][bank] < cfg.TOTAL_ROW_ACCESSES);
	case READ_P:
	case READ:
		return (bankState.currentBankState == RowActive &&
		        currentClockCycle >= bankState.nextRead &&
		        busPacket->row == bankState.openRowAddress &&
		        rowAccessCounters[rank][bank] < cfg.TOTAL_ROW_ACCESSES);
	case PRECHARGE:
		return (bankState.currentBankState == RowActive &&
		        currentClockCycle >= bankState.nextPrecharge);
	default:
		ERROR("== Error - Trying to issue a crazy bus packet type : " << *busPacket);
		abort();
	}
	return false;
}

//figures out if a rank's queue is empty
bool CommandQueue::isEmpty(unsigned rank) const
{
	if (cfg.QUEUING_STRUCTURE == PerRank)
	{
		return queues[rank][0].empty();
	}
	else if (cfg.QUEUING_STRUCTURE == PerRankPerBank)
	{
		for (size_t i=0;i<cfg.NUM_BANKS;i++)
		{
			if (!queues[rank][i].empty()) return false;
		}
		return true;
	}
	else
	{
		DEBUG("Invalid Queueing Stucture");
		abort();
	}
}

//tells the command queue that a particular rank is in need of a refresh
void CommandQueue::setRefreshNeeded(unsigned rank)
{
	refreshWaiting = true;
	refreshRank = rank;
}

void CommandQueue::nextRankAndBank(unsigned &rank, unsigned &bank)
{
	if (cfg.SCHEDULING_POLICY == RankThenBankRoundRobin)
	{
		rank++;
		if (rank == cfg.NUM_RANKS)
		{
			rank = 0;
			bank++;
			if (bank == cfg.NUM_BANKS)
			{
				bank = 0;
			}
		}
	}
	//bank-then-rank round robin
	else if (cfg.SCHEDULING_POLICY == BankThenRankRoundRobin)
	{
		bank++;
		if (bank == cfg.NUM_BANKS)
		{
			bank = 0;
			rank++;
			if (rank == cfg.NUM_RANKS)
			{
				rank = 0;
			}
		}
	}
	else
	{
		ERROR("== Error - Unknown scheduling policy");
		exit(0);
	}

}

void CommandQueue::update()
{
	//do nothing since pop() is effectively update(),
	//needed for SimulatorObject
	//TODO: make CommandQueue not a SimulatorObject
}
