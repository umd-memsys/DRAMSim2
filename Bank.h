#ifndef BANK_H
#define BANK_H

//Bank.h
//
//Header file for bank class
//

#include "SystemConfiguration.h"
#include "SimulatorObject.h"
#include "BankState.h"
#include "BusPacket.h"

namespace DRAMSim
{
	class Bank
	{
		typedef struct _DataStruct
		{
			uint row;
			void *data;
			struct _DataStruct *next;
		} DataStruct;

	public:
		//functions
		Bank();
		void read(BusPacket &busPacket);
		void write(const BusPacket &busPacket);

		//fields
		BankState currentState;

	private:
		// private member
		std::vector<DataStruct *> rowEntries; 

		static DataStruct *searchForRow(uint row, DataStruct *head);
	};
}

#endif

