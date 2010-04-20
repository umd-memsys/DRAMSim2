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
			void receiveFromBus(BusPacket &packet);
			void sendToBus(BusPacket &packet);
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
 			vector<BusPacket> readReturnPacket;
			vector<uint> readReturnCountdown;
			vector<BankState> bankStates;
	};
}
#endif

