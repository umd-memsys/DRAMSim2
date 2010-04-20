#ifndef BUSPACKET_H
#define BUSPACKET_H
//BusPacket.h
//
//Header file for bus packet object
//

#include "SystemConfiguration.h"

namespace DRAMSim
{
	enum BusPacketType
	{
		READ,
		READ_P,
		WRITE,
		WRITE_P,
		ACTIVATE,
		PRECHARGE,
		REFRESH,
		DATA
	};

	class BusPacket
	{
	public:
		//Fields
		BusPacketType busPacketType;
		uint column;
		uint row;
		uint bank;
		uint rank;
		uint64_t physicalAddress;
		void *data;

		//Functions
		BusPacket(BusPacketType packtype, uint64_t physicalAddr, uint col, uint rw, uint r, uint b, void *dat);
		BusPacket();

		void print();
		void print(uint64_t currentClockCycle, bool dataStart);
		static void printData(const void *data);
	};
}

#endif

