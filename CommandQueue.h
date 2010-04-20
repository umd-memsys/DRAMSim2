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
		typedef vector<BusPacket> BusPacket1D;
		typedef vector<BusPacket1D> BusPacket2D;
		typedef vector<BusPacket2D> BusPacket3D;

		//functions
		CommandQueue(vector< vector<BankState> > &states);
		CommandQueue();

		void enqueue(BusPacket newBusPacket, uint rank, uint bank);
		bool pop(BusPacket &busPacket);		 
		bool hasRoomFor(uint numberToEnqueue, uint rank, uint bank);
		bool isIssuable(BusPacket busPacket);
		bool isEmpty(uint rank);
		void needRefresh(uint rank);
		void print();
		void update(); //SimulatorObject requirement

		//fields
		BusPacket3D queues;
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

