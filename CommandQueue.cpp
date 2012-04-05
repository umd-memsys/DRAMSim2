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

#include "CommandQueue.h"
#include "MemoryController.h"

using namespace DRAMSim;

CommandQueue::CommandQueue(vector< vector<BankState> > &states) :
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
	if (queuingStructure==PerRank)
	{
		numBankQueues = 1;
	}
	else if (queuingStructure==PerRankPerBank)
	{
		numBankQueues = NUM_BANKS;
	}
	else
	{
		ERROR("== Error - Unknown queuing structure");
		exit(0);
	}

	//vector of counters used to ensure rows don't stay open too long
	rowAccessCounters = vector< vector<unsigned> >(NUM_RANKS, vector<unsigned>(NUM_BANKS,0));

	//create queue based on the structure we want
	BusPacket1D actualQueue;
	BusPacket2D perBankQueue = BusPacket2D();
	queues = BusPacket3D();
	for (size_t rank=0; rank<NUM_RANKS; rank++)
	{
		//this loop will run only once for per-rank and NUM_BANKS times for per-rank-per-bank
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
	tFAWCountdown.reserve(NUM_RANKS);
	for (size_t i=0;i<NUM_RANKS;i++)
	{
		//init the empty vectors here so we don't seg fault later
		tFAWCountdown.push_back(vector<unsigned>());
	}
}
CommandQueue::~CommandQueue()
{
	//ERROR("COMMAND QUEUE destructor");
	size_t bankMax = NUM_RANKS;
	if (queuingStructure == PerRank) {
		bankMax = 1; 
	}
	for (size_t r=0; r< NUM_RANKS; r++)
	{
		for (size_t b=0; b<bankMax; b++) 
		{
			for (size_t i=0; i<queues[r][b].size(); i++)
			{
				delete(queues[r][b][i]);
			}
			queues[r][b].clear();
		}
	}
}
//Adds a command to appropriate queue
void CommandQueue::enqueue(BusPacket *newBusPacket)
{
	unsigned rank = newBusPacket->rank;
	unsigned bank = newBusPacket->bank;
	if (queuingStructure==PerRank)
	{
		queues[rank][0].push_back(newBusPacket);
		if (queues[rank][0].size()>CMD_QUEUE_DEPTH)
		{
			ERROR("== Error - Enqueued more than allowed in command queue");
			ERROR("						Need to call .hasRoomFor(int numberToEnqueue, unsigned rank, unsigned bank) first");
			exit(0);
		}
	}
	else if (queuingStructure==PerRankPerBank)
	{
		queues[rank][bank].push_back(newBusPacket);
		if (queues[rank][bank].size()>CMD_QUEUE_DEPTH)
		{
			ERROR("== Error - Enqueued more than allowed in command queue");
			ERROR("						Need to call .hasRoomFor(int numberToEnqueue, unsigned rank, unsigned bank) first");
			exit(0);
		}
	}
	else
	{
		ERROR("== Error - Unknown queuing structure");
		exit(0);
	}
}

//Removes the next item from the command queue based on the system's
//command scheduling policy
bool CommandQueue::pop(BusPacket **busPacket)
{
	//this can be done here because pop() is called every clock cycle by the parent MemoryController
	//	figures out the sliding window requirement for tFAW
	//
	//deal with tFAW book-keeping
	//	each rank has it's own counter since the restriction is on a device level
	for (size_t i=0;i<NUM_RANKS;i++)
	{
		//decrement all the counters we have going
		for (size_t j=0;j<tFAWCountdown[i].size();j++)
		{
			tFAWCountdown[i][j]--;
		}

		//the head will always be the smallest counter, so check if it has reached 0
		if (tFAWCountdown[i].size()>0 && tFAWCountdown[i][0]==0)
		{
			tFAWCountdown[i].erase(tFAWCountdown[i].begin());
		}
	}

	//
	//Dequeue the correct item based on the structure and whether
	//	or not we are using open or closed page
	//
	if (queuingStructure==PerRank)
	{
		if (rowBufferPolicy==ClosePage)
		{
			bool sendingREF = false;
			//if the memory controller set the flags signaling that we need to issue a refresh
			if (refreshWaiting)
			{
				bool foundActiveOrTooEarly = false;
				//look for an open bank
				for (size_t i=0;i<NUM_BANKS;i++)
				{
					//checks to make sure that all banks are idle
					if (bankStates[refreshRank][i].currentBankState == RowActive)
					{
						foundActiveOrTooEarly = true;
						//if a bank is open, make sure there are no commands pending that go to the
						//  open row
						for (size_t j=0;j<queues[refreshRank][0].size();j++)
						{
							if (queues[refreshRank][0][j]->row == bankStates[refreshRank][i].openRowAddress &&
							        queues[refreshRank][0][j]->bank == i)
							{
								if (queues[refreshRank][0][j]->busPacketType != ACTIVATE &&
								        isIssuable(queues[refreshRank][0][j]))
								{
									*busPacket = queues[refreshRank][0][j];
									queues[refreshRank][0].erase(queues[refreshRank][0].begin()+j);
									sendingREF = true;
								}
								break;
							}
						}

						break;
					}
					//	NOTE: checks nextActivate time for each bank to make sure tRP is being
					//				satisfied.	the next ACT and next REF can be issued at the same
					//				point in the future, so just use nextActivate field instead of
					//				creating a nextRefresh field
					else if (bankStates[refreshRank][i].nextActivate > currentClockCycle)
					{
						foundActiveOrTooEarly = true;
						break;
					}
				}

				//if there are no open banks and timing has been met, send out the refresh
				//	reset flags and rank pointer
				if (!foundActiveOrTooEarly && bankStates[refreshRank][0].currentBankState != PowerDown)
				{
					*busPacket = new BusPacket(REFRESH, 0, 0, 0, refreshRank, 0, 0);
					refreshRank = -1;
					refreshWaiting = false;
					sendingREF = true;
				}
			}

			//if we're not sending a refresh
			if (!sendingREF)
			{
				bool foundIssuable = false;
				unsigned startingRank = nextRank;
				do
				{
					//make sure there is something in this queue first
					//	also make sure a rank isn't waiting for a refresh
					//	if a rank is waiting for a refesh, don't issue anything to it until the
					//		refresh logic above has sent one out (ie, letting banks close)
					if (!queues[nextRank][0].empty() && !((nextRank == refreshRank) && refreshWaiting))
					{
						//search from beginning to find first issuable bus packet
						for (size_t i=0;i<queues[nextRank][0].size();i++)
						{
							if (isIssuable(queues[nextRank][0][i]))
							{
								//check to make sure we aren't removing a read/write that is paired with an activate
								if (i>0 && queues[nextRank][0][i-1]->busPacketType==ACTIVATE &&
								        queues[nextRank][0][i-1]->physicalAddress == queues[nextRank][0][i]->physicalAddress)
									continue;

								*busPacket = queues[nextRank][0][i];
								queues[nextRank][0].erase(queues[nextRank][0].begin()+i);
								foundIssuable = true;
								break;
							}
						}

					}

					//if we found something, break out of do-while
					if (foundIssuable) break;

					//rank round robin
					nextRank++;
					if (nextRank == NUM_RANKS)
					{
						nextRank = 0;
					}
				}
				while (startingRank != nextRank);

				//if we couldn't find anything to send, return false
				if (!foundIssuable) return false;
			}
		}
		//if we are open page, we will want to search the queues for shit going to same row
		else if (rowBufferPolicy==OpenPage)
		{
			bool sendingREForPRE = false;
			if (refreshWaiting)
			{
				bool sendREF = true;
				//make sure we meet all the requirements to send a REF
				for (size_t b=0;b<NUM_BANKS;b++)
				{
					//if a bank is active we can't send a REF yet
					if (bankStates[refreshRank][b].currentBankState == RowActive)
					{
						sendREF = false;
						bool closeRow = true;
						//search for commands going to an open row
						vector <BusPacket *> &refreshQueue = queues[refreshRank][0];

						for (size_t j=0;j<refreshQueue.size();j++)
						{
							BusPacket *packet = refreshQueue[j];
							//if a command in the queue is going to the same row . . .
							if (bankStates[refreshRank][b].openRowAddress == packet->row &&
							        b == packet->bank)
							{
								// . . . and is not an activate . . .
								if (packet->busPacketType != ACTIVATE)
								{
									closeRow = false;
									// . . . and can be issued . . .
									if (isIssuable(packet))
									{
										//send it out
										*busPacket = packet;
										refreshQueue.erase(refreshQueue.begin()+j);
										sendingREForPRE = true;
									}
									break;
								}
								else //command is an activate
								{
									break;
								}
							}
						}

						//if the bank is open and we are allowed to close it, then send a PRE
						if (closeRow && currentClockCycle >= bankStates[refreshRank][b].nextPrecharge)
						{
							rowAccessCounters[refreshRank][b]=0;
							*busPacket = new BusPacket(PRECHARGE, 0, 0, 0, refreshRank, b, 0);
							sendingREForPRE = true;
						}
						break;
					}
					//	NOTE: the next ACT and next REF can be issued at the same
					//				point in the future, so just use nextActivate field instead of
					//				creating a nextRefresh field
					else if (bankStates[refreshRank][b].nextActivate > currentClockCycle) //and this bank doesn't have an open row
					{
						sendREF = false;
						break;
					}
				}

				//if there are no open banks and timing has been met, send out the refresh
				//	reset flags and rank pointer
				if (sendREF && bankStates[refreshRank][0].currentBankState != PowerDown)
				{
					*busPacket = new BusPacket(REFRESH, 0, 0, 0, refreshRank, 0, 0);
					refreshRank = -1;
					refreshWaiting = false;
					sendingREForPRE = true;
				}
			}

			if (!sendingREForPRE)
			{
				unsigned startingRank = nextRank;
				bool foundIssuable = false;
				do
				{
					//make sure there is something there first
					if (!queues[nextRank][0].empty() && !((nextRank == refreshRank) && refreshWaiting))
					{
						//search from the beginning to find first issuable bus packet
						for (size_t i=0;i<queues[nextRank][0].size();i++)
						{
							if (isIssuable(queues[nextRank][0][i]))
							{
								//check for dependencies
								bool dependencyFound = false;
								for (size_t j=0;j<i;j++)
								{
									if (queues[nextRank][0][j]->busPacketType != ACTIVATE &&
									        queues[nextRank][0][j]->bank == queues[nextRank][0][i]->bank &&
									        queues[nextRank][0][j]->row == queues[nextRank][0][i]->row)
									{
										dependencyFound = true;
										break;
									}
								}
								if (dependencyFound) continue;

								*busPacket = queues[nextRank][0][i];

								//if the bus packet before is an activate, that is the act that was
								//	paired with the column access we are removing, so we have to remove
								//	that activate as well (check i>0 because if i==0 then theres nothing before it)

								if (i>0 && queues[nextRank][0][i-1]->busPacketType == ACTIVATE)
								{
									rowAccessCounters[(*busPacket)->rank][(*busPacket)->bank]++;
									// i is being returned, but i-1 is being thrown away, so must delete it here 
									delete(queues[nextRank][0][i-1]);
									//erase is exclusive on the upper end, so really this will erase (i-1) and i
									queues[nextRank][0].erase(queues[nextRank][0].begin()+i-1,queues[nextRank][0].begin()+i+1);
								}
								else
								{
									//or just remove the one bus packet
									queues[nextRank][0].erase(queues[nextRank][0].begin()+i);
								}

								foundIssuable = true;
								break;
							}
						}
					}

					//if we found something, break out of do-while
					if (foundIssuable) break;

					//rank round robin
					nextRank++;
					if (nextRank == NUM_RANKS)
					{
						nextRank = 0;
					}
				}
				while (startingRank != nextRank);

				//if nothing was issuable, see if we can issue a PRE to an open bank
				//	that has no other commands waiting
				if (!foundIssuable)
				{
					//search for banks to close
					bool sendingPRE = false;
					unsigned startingRank = nextRankPRE;
					unsigned startingBank = nextBankPRE;
					do
					{
						bool found = false;
						//check if bank is open
						if (bankStates[nextRankPRE][nextBankPRE].currentBankState == RowActive)
						{
							for (size_t i=0;i<queues[nextRankPRE][0].size();i++)
							{
								//if there is something going to that bank and row, then we don't want to send a PRE
								if (queues[nextRankPRE][0][i]->bank == nextBankPRE &&
								        queues[nextRankPRE][0][i]->row == bankStates[nextRankPRE][nextBankPRE].openRowAddress)
								{
									found = true;
									break;
								}
							}

							//if nothing found going to that bank and row or too many accesses have happend, close it
							if (!found || rowAccessCounters[nextRankPRE][nextBankPRE]==TOTAL_ROW_ACCESSES)
							{
								if (currentClockCycle >= bankStates[nextRankPRE][nextBankPRE].nextPrecharge)
								{
									sendingPRE = true;
									rowAccessCounters[nextRankPRE][nextBankPRE]=0;
									*busPacket = new BusPacket(PRECHARGE, 0, 0, 0, nextRankPRE, nextBankPRE, 0);
									break;
								}
							}
						}
						nextRankAndBank(nextRankPRE, nextBankPRE);		
					}
					while (!(startingRank == nextRankPRE && startingBank == nextBankPRE));

					//if no PRE to send, just return false
					if (!sendingPRE) return false;
				}
			}
		}
	}
	else if (queuingStructure==PerRankPerBank)
	{
		if (rowBufferPolicy==ClosePage)
		{
			bool sendingREF = false;
			if (refreshWaiting)
			{
				bool foundActiveOrTooEarly = false;
				//look for open banks
				for (size_t i=0;i<NUM_BANKS;i++)
				{
					//checks to make sure that all banks are idle
					if (bankStates[refreshRank][i].currentBankState == RowActive)
					{
						foundActiveOrTooEarly = true;

						//if the bank is open, make sure there is nothing else
						// going there before we close it
						for (size_t j=0;j<queues[refreshRank][i].size();j++)
						{
							if (queues[refreshRank][i][j]->row == bankStates[refreshRank][i].openRowAddress)
							{
								if (queues[refreshRank][i][j]->busPacketType != ACTIVATE &&
								        isIssuable(queues[refreshRank][i][j]))
								{
									*busPacket = queues[refreshRank][i][j];
									queues[refreshRank][i].erase(queues[refreshRank][i].begin() + j);
									sendingREF = true;
								}
								break;
							}
						}

						break;
					}
					//	NOTE: checks nextActivate time for each bank to make sure tRP is being
					//				satisfied.	the next ACT and next REF can be issued at the same
					//				point in the future, so just use nextActivate field instead of
					//				creating a nextRefresh field
					else if (bankStates[refreshRank][i].nextActivate > currentClockCycle)
					{
						foundActiveOrTooEarly = true;
						break;
					}
				}

				//if all banks are idle and timing has been met, send a refresh and
				//	reset flags and pointers
				if (!foundActiveOrTooEarly && bankStates[refreshRank][0].currentBankState != PowerDown)
				{
					*busPacket = new BusPacket(REFRESH, 0, 0, 0, refreshRank, 0, 0);
					refreshRank = -1;
					refreshWaiting = false;
					sendingREF = true;
				}
			}

			//if we're not sending a REF, proceed as normal
			if (!sendingREF)
			{
				unsigned startingRank = nextRank;
				unsigned startingBank = nextBank;
				bool foundIssuable = false;
				do
				{
					//check if something is there first
					if (!queues[nextRank][nextBank].empty() && !((nextRank == refreshRank) && refreshWaiting))
					{
						if (isIssuable(queues[nextRank][nextBank][0]))
						{
							//no need to search because if the front can't be sent,
							// then no chance something behind it can go instead
							*busPacket = queues[nextRank][nextBank][0];
							queues[nextRank][nextBank].erase(queues[nextRank][nextBank].begin());
							foundIssuable = true;
							break;
						}
					}
					nextRankAndBank(nextRank, nextBank);
				}
				while (!(startingRank == nextRank && startingBank == nextBank));

				//if nothing was found that could go, just return false
				if (!foundIssuable) return false;
			}
		}

		else if (rowBufferPolicy==OpenPage)
		{
			bool sendingREForPRE = false;
			if (refreshWaiting)
			{
				bool sendREF = true;
				//make sure all banks idle and timing met
				for (size_t i=0;i<NUM_BANKS;i++)
				{
					//if a bank is active we can't send a REF yet
					if (bankStates[refreshRank][i].currentBankState == RowActive)
					{
						sendREF = false;
						bool closeRow = true;
						//search for commands going to open bank
						for (size_t j=0;j<queues[refreshRank][i].size();j++)
						{
							if (bankStates[refreshRank][i].openRowAddress == queues[refreshRank][i][j]->row)
							{
								if (queues[refreshRank][i][j]->busPacketType != ACTIVATE)
								{
									closeRow = false;
									if (isIssuable(queues[refreshRank][i][j]))
									{
										*busPacket = queues[refreshRank][i][j];
										queues[refreshRank][i].erase(queues[refreshRank][i].begin()+j);
										sendingREForPRE=true;
									}
									break;
								}
								else
								{
									//if we've encountered another act, no other command will be of interest
									break;
								}
							}
						}

						//if the bank is open and we are allowed to close it, then send a PRE
						if (closeRow && currentClockCycle >= bankStates[refreshRank][i].nextPrecharge)
						{
							rowAccessCounters[refreshRank][i]=0;

							*busPacket = new BusPacket(PRECHARGE, 0, 0, 0, refreshRank, i, 0);
							sendingREForPRE = true;
						}
						break;
					}
					//	NOTE: the next ACT and next REF can be issued at the same
					//				point in the future, so just use nextActivate field instead of
					//				creating a nextRefresh field
					else if (bankStates[refreshRank][i].nextActivate > currentClockCycle)
					{
						sendREF = false;
						break;
					}
				}

				//if there are no open banks and timing has been met, send out the refresh
				//	reset flags and rank pointer
				if (sendREF && bankStates[refreshRank][0].currentBankState != PowerDown)
				{
					*busPacket = new BusPacket(REFRESH, 0, 0, 0, refreshRank, 0, 0);
					refreshRank = -1;
					refreshWaiting = false;
					sendingREForPRE = true;
				}
			}

			if (!sendingREForPRE)
			{
				unsigned startingRank = nextRank;
				unsigned startingBank = nextBank;
				bool foundIssuable = false;
				do
				{
					//check to see if something is there first
					if (!queues[nextRank][nextBank].empty() && !((nextRank == refreshRank) && refreshWaiting))
					{
						//search from the beginning to find first issuable
						for (size_t i=0;i<queues[nextRank][nextBank].size();i++)
						{
							if (isIssuable(queues[nextRank][nextBank][i]))
							{
								//check for dependencies
								bool dependencyFound = false;
								for (size_t j=0;j<i;j++)
								{
									if (queues[nextRank][nextBank][j]->busPacketType != ACTIVATE &&
									        queues[nextRank][nextBank][i]->row == queues[nextRank][nextBank][j]->row)
									{
										dependencyFound = true;
										break;
									}
								}
								if (dependencyFound) continue;

								*busPacket = queues[nextRank][nextBank][i];

								//if the bus packet before is an activate, that is the act that was
								//	paired with the column access we are removing, so we have to remove
								//	that activate as well (check i>0 because if i==0 then theres nothing before it)
								if (i>0 && queues[nextRank][nextBank][i-1]->busPacketType == ACTIVATE)
								{
									rowAccessCounters[nextRank][nextBank]++;
									// (i-1) is thrown away here so get rid of it
									delete (queues[nextRank][nextBank][i-1]);

									//erase both i-1 and i
									queues[nextRank][nextBank].erase(queues[nextRank][nextBank].begin()+i-1,
									                                 queues[nextRank][nextBank].begin()+i+1);
								}
								else
								{
									//or just erase the one
									queues[nextRank][nextBank].erase(queues[nextRank][nextBank].begin()+i);
								}

								foundIssuable = true;
								break;
							}
						}
					}

					//if we found something, break out of do-while
					if (foundIssuable) break;
					nextRankAndBank(nextRank, nextBank); 

				}
				while (!(startingRank == nextRank && startingBank == nextBank));

				//if nothing was issuable, see if we can issue a PRE to an open bank
				//	that has no other commands waiting
				if (!foundIssuable)
				{
					bool sendingPRE = false;
					bool startingRank = nextRankPRE;
					bool startingBank = nextBankPRE;
					do
					{
						bool found = false;
						//check to see if bank is open
						if (bankStates[nextRankPRE][nextBankPRE].currentBankState == RowActive)
						{
							for (size_t i=0;i<queues[nextRankPRE][nextBankPRE].size();i++)
							{
								//if something is going to the open row, we shouldn't close it
								if (queues[nextRankPRE][nextBankPRE][i]->row == bankStates[nextRankPRE][nextBankPRE].openRowAddress)
								{
									found = true;
									break;
								}
							}

							//if nothing was found going to the open row, send a PRE
							if (!found || rowAccessCounters[nextRankPRE][nextBankPRE]==TOTAL_ROW_ACCESSES)
							{
								if (currentClockCycle >= bankStates[nextRankPRE][nextBankPRE].nextPrecharge)
								{
									rowAccessCounters[nextRankPRE][nextBankPRE] = 0;

									sendingPRE = true;
									*busPacket = new BusPacket(PRECHARGE, 0, 0, 0, nextRankPRE, nextBankPRE, 0);
									break;
								}
							}
						}
						nextRankAndBank(nextRankPRE, nextBankPRE);
					}
					while (!(startingRank == nextRankPRE && startingBank == nextBankPRE));

					//if no PREs could be sent, just return false
					if (!sendingPRE) return false;
				}
			}
		}
	}
	else
	{
		ERROR("== Error - Unknown queuing structure");
		exit(0);
	}


	//sendAct is flag used for posted-cas
	//  posted-cas is enabled when AL>0
	//  when sendAct is true, when don't want to increment our indexes
	//  so we send the column access that is paid with this act
	if (AL>0 && sendAct)
	{
		sendAct = false;
	}
	else
	{
		sendAct = true;
		//Using rank-then-bank round-robin policy, determine which queue we will pull from next
		if (schedulingPolicy == RankThenBankRoundRobin)
		{
			nextRank++;
			if (nextRank == NUM_RANKS)
			{
				nextRank = 0;
				nextBank++;
				if (nextBank == NUM_BANKS)
				{
					nextBank = 0;
				}
			}
		}
		//bank-then-rank round robin
		else if (schedulingPolicy == BankThenRankRoundRobin)
		{
			nextBank++;
			if (nextBank == NUM_BANKS)
			{
				nextBank = 0;
				nextRank++;
				if (nextRank == NUM_RANKS)
				{
					nextRank = 0;
				}
			}
		}
		else
		{
			ERROR("== Error - Unknown scheduling policy");
			exit(0);
		}
	}

	//if its an activate, add a tfaw counter
	if ((*busPacket)->busPacketType==ACTIVATE)
	{
		tFAWCountdown[(*busPacket)->rank].push_back(tFAW);
	}

	return true;
}

//check if a rank/bank queue has room for a certain number of bus packets
bool CommandQueue::hasRoomFor(unsigned numberToEnqueue, unsigned rank, unsigned bank)
{
	if (queuingStructure == PerRank)
	{
		if (CMD_QUEUE_DEPTH - queues[rank][0].size() >= numberToEnqueue)
		{
			return true;
		}
		else return false;
	}
	else if (queuingStructure == PerRankPerBank)
	{
		if (CMD_QUEUE_DEPTH - queues[rank][bank].size() >= numberToEnqueue)
		{
			return true;
		}
		else return false;
	}

	return false;
}

//prints the contents of the command queue
void CommandQueue::print()
{
	if (queuingStructure==PerRank)
	{
		PRINT(endl << "== Printing Per Rank Queue" );
		for (size_t i=0;i<NUM_RANKS;i++)
		{
			PRINT(" = Rank " << i << "  size : " << queues[i][0].size() );
			for (size_t j=0;j<queues[i][0].size();j++)
			{
				PRINTN("    "<< j << "]");
				queues[i][0][j]->print();
			}
		}
	}
	else if (queuingStructure==PerRankPerBank)
	{
		PRINT("\n== Printing Per Rank, Per Bank Queue" );

		for (size_t i=0;i<NUM_RANKS;i++)
		{
			PRINT(" = Rank " << i );
			for (size_t j=0;j<NUM_BANKS;j++)
			{
				PRINT("    Bank "<< j << "   size : " << queues[i][j].size() );

				for (size_t k=0;k<queues[i][j].size();k++)
				{
					PRINTN("       " << k << "]");
					queues[i][j][k]->print();
				}
			}
		}
	}
}

//checks if busPacket is allowed to be issued
bool CommandQueue::isIssuable(BusPacket *busPacket)
{
	switch (busPacket->busPacketType)
	{
	case REFRESH:

		break;
	case ACTIVATE:
		if ((bankStates[busPacket->rank][busPacket->bank].currentBankState == Idle ||
		        bankStates[busPacket->rank][busPacket->bank].currentBankState == Refreshing) &&
		        currentClockCycle >= bankStates[busPacket->rank][busPacket->bank].nextActivate &&
		        tFAWCountdown[busPacket->rank].size() < 4)
		{
			return true;
		}
		else
		{
			return false;
		}
		break;
	case WRITE:
	case WRITE_P:
		if (bankStates[busPacket->rank][busPacket->bank].currentBankState == RowActive &&
		        currentClockCycle >= bankStates[busPacket->rank][busPacket->bank].nextWrite &&
		        busPacket->row == bankStates[busPacket->rank][busPacket->bank].openRowAddress &&
		        rowAccessCounters[busPacket->rank][busPacket->bank] < TOTAL_ROW_ACCESSES)
		{
			return true;
		}
		else
		{
			return false;
		}
		break;
	case READ_P:
	case READ:
		if (bankStates[busPacket->rank][busPacket->bank].currentBankState == RowActive &&
		        currentClockCycle >= bankStates[busPacket->rank][busPacket->bank].nextRead &&
		        busPacket->row == bankStates[busPacket->rank][busPacket->bank].openRowAddress &&
		        rowAccessCounters[busPacket->rank][busPacket->bank] < TOTAL_ROW_ACCESSES)
		{
			return true;
		}
		else
		{
			return false;
		}
		break;
	case PRECHARGE:
		if (bankStates[busPacket->rank][busPacket->bank].currentBankState == RowActive &&
		        currentClockCycle >= bankStates[busPacket->rank][busPacket->bank].nextPrecharge)
		{
			return true;
		}
		else
		{
			return false;
		}
		break;
	default:
		ERROR("== Error - Trying to issue a crazy bus packet type : ");
		busPacket->print();
		exit(0);
	}
	return false;
}

//figures out if a rank's queue is empty
bool CommandQueue::isEmpty(unsigned rank)
{
	if (queuingStructure == PerRank)
	{
		return queues[rank][0].empty();
	}
	else if (queuingStructure == PerRankPerBank)
	{
		for (size_t i=0;i<NUM_BANKS;i++)
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
void CommandQueue::needRefresh(unsigned rank)
{
	refreshWaiting = true;
	refreshRank = rank;
}

void CommandQueue::nextRankAndBank(unsigned &rank, unsigned &bank)
{
	if (schedulingPolicy == RankThenBankRoundRobin)
	{
		rank++;
		if (rank == NUM_RANKS)
		{
			rank = 0;
			bank++;
			if (bank == NUM_BANKS)
			{
				bank = 0;
			}
		}
	}
	//bank-then-rank round robin
	else if (schedulingPolicy == BankThenRankRoundRobin)
	{
		bank++;
		if (bank == NUM_BANKS)
		{
			bank = 0;
			rank++;
			if (rank == NUM_RANKS)
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
