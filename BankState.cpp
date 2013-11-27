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
#include "BankState.h"

using namespace std;
using namespace DRAMSim;

//All banks start precharged
BankState::BankState(const Config &cfg_)
	: cfg(cfg_),
		currentBankState(Idle),
		openRowAddress(0),
		nextRead(0),
		nextWrite(0),
		nextActivate(0),
		nextPrecharge(0),
		nextPowerUp(0),
		lastCommand(READ),
		stateChangeCountdown(0)
{}

void BankState::updateState(const BusPacket &bp, uint64_t currentClockCycle) {

	lastCommand = bp.busPacketType;

	if (bp.busPacketType == READ_P) {
		nextActivate = max(currentClockCycle + cfg.READ_AUTOPRE_DELAY, nextActivate);
		stateChangeCountdown = cfg.READ_TO_PRE_DELAY;

		//set read and write to nextActivate so the state table will prevent a read or write
		//  being issued (in cq.isIssuable())before the bank state has been changed because of the
		//  auto-precharge associated with this command
		nextRead = nextActivate;
		nextWrite = nextActivate;
	}

	else if (bp.busPacketType == READ) {
		nextPrecharge = max(currentClockCycle + cfg.READ_TO_PRE_DELAY, nextPrecharge);
	}

	else if (bp.busPacketType == WRITE_P) {
		nextActivate = max(currentClockCycle + cfg.WRITE_AUTOPRE_DELAY, nextActivate);
		stateChangeCountdown = cfg.WRITE_TO_PRE_DELAY;

		//set read and write to nextActivate so the state table will prevent a read or write
		//  being issued (in cq.isIssuable())before the bank state has been changed because of the
		//  auto-precharge associated with this command
		nextRead = nextActivate;
		nextWrite = nextActivate;
	}

	else if (bp.busPacketType == WRITE) {
		nextPrecharge = max(currentClockCycle + cfg.WRITE_TO_PRE_DELAY, nextPrecharge);
	}

	else if (bp.busPacketType == ACTIVATE) {
		currentBankState = RowActive;
		openRowAddress = bp.row;
		nextActivate = max(currentClockCycle + cfg.tRC, nextActivate);
		nextPrecharge = max(currentClockCycle + cfg.tRAS, nextPrecharge);

		//if we are using posted-CAS, the next column access can be sooner than normal operation

		nextRead = max(currentClockCycle + (cfg.tRCD-cfg.AL), nextRead);
		nextWrite = max(currentClockCycle + (cfg.tRCD-cfg.AL), nextWrite);
	}

	else if (bp.busPacketType == PRECHARGE) {
		currentBankState = Precharging;
		stateChangeCountdown = cfg.tRP;
		nextActivate = max(currentClockCycle + cfg.tRP, nextActivate);
	}
	else if (bp.busPacketType == REFRESH) {
		nextActivate = currentClockCycle + cfg.tRFC;
		currentBankState = Refreshing;
		stateChangeCountdown = cfg.tRFC;
	}

	else if (bp.busPacketType == DATA) {
		// no-op 
	}

	else {
		ERROR("== Error - Popped a command we shouldn't have of type : " << bp);
		abort();
	}
}

void BankState::updateStateChange() {
	if (stateChangeCountdown>0)
	{
		//decrement counters
		stateChangeCountdown--;

		//if counter has reached 0, change state
		if (stateChangeCountdown == 0)
		{
			switch (lastCommand)
			{
				//only these commands have an implicit state change
				case WRITE_P:
				case READ_P:
					currentBankState = Precharging;
					lastCommand = PRECHARGE;
					stateChangeCountdown = cfg.tRP;
					break;

				case REFRESH:
				case PRECHARGE:
					currentBankState = Idle;
					break;
				default:
					break;
			}
		}
	}
}

ostream &BankState::print(ostream &out) const
{
	out << " == Bank State \n";
	out << "    State : ";
	if (currentBankState == Idle)
		out << "Idle";
	else if (currentBankState == RowActive)
		out << "Active";
	else if (currentBankState == Refreshing)
		out << "Refreshing";
	else if (currentBankState == PowerDown)
		out << "Power Down";
	out <<"\n";

	out << "    OpenRowAddress : " << openRowAddress <<"\n";
	out << "    nextRead       : " << nextRead <<"\n";
	out << "    nextWrite      : " << nextWrite <<"\n";
	out << "    nextActivate   : " << nextActivate <<"\n";
	out << "    nextPrecharge  : " << nextPrecharge <<"\n";
	out << "    nextPowerUp    : " << nextPowerUp <<"\n";
	return out;
}

namespace DRAMSim {
	ostream &operator<<(ostream &out, const BankState &bankState) {
		return bankState.print(out);
	}
}
