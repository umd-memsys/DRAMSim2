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








//BankState.cpp
//
//Class file for bank state object
//

#include "BankState.h"

using namespace std;
using namespace DRAMSim;

//All banks start precharged
BankState::BankState():
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
