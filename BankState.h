#ifndef BANKSTATE_H
#define BANKSTATE_H

//BankState.h
//
//Header file for bank state class
//

#include "SystemConfiguration.h"
#include "BusPacket.h"

namespace DRAMSim
{
	enum CurrentBankState
	{
		Idle,
		RowActive,
		Precharging,
		Refreshing,
		PowerDown
	};

	class BankState
	{
	public:
		//Fields
		CurrentBankState currentBankState;
		uint openRowAddress;
		uint64_t nextRead;
		uint64_t nextWrite;
		uint64_t nextActivate;
		uint64_t nextPrecharge;
		uint64_t nextPowerUp;

		BusPacketType lastCommand;
		uint stateChangeCountdown;

		//Functions
		BankState();
		void print();
	};
}

#endif

