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

//BusPacket.cpp
//
//Class file for bus packet object
//

#include "ConfigIniReader.h"
#include "PrintMacros.h"
#include "BusPacket.h"
#include "Transaction.h"

using namespace DRAMSim;
using namespace std;

BusPacket::BusPacket()
	: busPacketType(READ),
	column(0),
	row(0),
	bank(0),
	rank(0),
	physicalAddress(0),
	data(0), 
	sourceTransaction(NULL), 
	// don't bother accounting for channels since they are truly independent
	globalBankId(0), 
	globalRowId(0)
{}

void BusPacket::init (BusPacketType packtype, uint64_t physicalAddr, 
		unsigned col, unsigned rw, unsigned r, unsigned b, const Config &cfg, void *dat) {

	busPacketType=packtype;
	column=col;
	row=rw;
	bank=b;
	rank=r;
	physicalAddress=physicalAddr;
	data=dat;
	sourceTransaction=NULL;
	// don't bother accounting for channels since they are truly independent
	globalBankId=rank*cfg.NUM_BANKS + bank;
	globalRowId=globalBankId * cfg.NUM_ROWS + row;
}

void BusPacket::setSourceTransaction(Transaction *t) {
	assert(t);
	assert(this);
	sourceTransaction = t; 
	t->addBusPacket(this);
}

void BusPacket::setDependsOn(BusPacket *dependentBP) {
	//std::cout << "Setting "<<*this<<"    depends on    "<<*dependentBP<<"\n";
	if (dependentBP->sourceTransaction && this->sourceTransaction &&
			dependentBP->sourceTransaction->transactionId == this->sourceTransaction->transactionId) {
		ERROR("Setting circular transaction dependency");
		abort();
	}

	this->dependencies.push_back(dependentBP); 	
	dependentBP->addNotifyList(this);
}
void BusPacket::clearDependency(const BusPacket *dependentBP) {
	unsigned numRemoved = 0;
	//std::cout <<"Removing dep "<<*dependentBP<<" from "<<*this<<" with dependencies: \n";

	// should only be one dep in our list, so might be enough to just return after the erase()
	for (vector<BusPacket *>::iterator it = dependencies.begin(); it != dependencies.end(); ) {
	//	std::cout << "\t - " << **it <<"\n"; 
		if ((*it)->sourceTransaction) {
			assert((*it)->sourceTransaction->valid);
		}

		if (*it == dependentBP) {
			it = dependencies.erase(it);
			numRemoved++;
		} else {
			++it;
		}
	}
	assert(numRemoved ==1);
}

bool BusPacket::isDependent(BusPacket *other) const {
	if (this == other) {
		abort();
	}

	// only check between actual data access commands
	if (!this->isCAS() || !other->isCAS()) {
		return false; 
	}

	// otherwise, the most likely case is they simply aren't going to the same bank
	if (this->globalBankId != other->globalBankId) {
		return false;
	}

	// all requests to same row must be serialized
	if (this->globalRowId == other->globalRowId) {
		// enforce write dependence (RAW, WAR, WAW) 
		if (this->isWrite() || other->isWrite()) {
			return true;
		}
	}

	// TODO: any other cases: going to same bank, but different rows
	return false; 
}


void BusPacket::print(uint64_t currentClockCycle, bool dataStart)
{
	// FIXME: move this out of BP 
	abort();
#if 0 
	if (this == NULL)
	{
		return;
	}

	if (cfg.VERIFICATION_OUTPUT)
	{
		switch (busPacketType)
		{
		case READ:
			cmd_verify_out << currentClockCycle << ": read ("<<rank<<","<<bank<<","<<column<<",0);"<<endl;
			break;
		case READ_P:
			cmd_verify_out << currentClockCycle << ": read ("<<rank<<","<<bank<<","<<column<<",1);"<<endl;
			break;
		case WRITE:
			cmd_verify_out << currentClockCycle << ": write ("<<rank<<","<<bank<<","<<column<<",0 , 0, 'h0);"<<endl;
			break;
		case WRITE_P:
			cmd_verify_out << currentClockCycle << ": write ("<<rank<<","<<bank<<","<<column<<",1, 0, 'h0);"<<endl;
			break;
		case ACTIVATE:
			cmd_verify_out << currentClockCycle <<": activate (" << rank << "," << bank << "," << row <<");"<<endl;
			break;
		case PRECHARGE:
			cmd_verify_out << currentClockCycle <<": precharge (" << rank << "," << bank << "," << row <<");"<<endl;
			break;
		case REFRESH:
			cmd_verify_out << currentClockCycle <<": refresh (" << rank << ");"<<endl;
			break;
		case DATA:
			//TODO: data verification?
			break;
		default:
			ERROR("Trying to print unknown kind of bus packet");
			exit(-1);
		}
	}
#endif 

}

ostream &BusPacket::print(ostream &out) const {
	out << "BP ("<< hex<<this<<dec<<") ";
	if (sourceTransaction) {
		out << "TID [" <<sourceTransaction->transactionId<<", "<<hex<<sourceTransaction<<"] ";
	}

	out <<"[";
	switch (busPacketType)
	{
		case READ:
			out << "READ";
			break;
		case READ_P:
			out << "READ_P";
			break; 
		case WRITE:
			out << "WRITE";
			break;
		case WRITE_P:
			out << "WRITE_P"; 
			break;
		case ACTIVATE:
			out << "ACT"; 
			break;
		case PRECHARGE:
			out << "PRE";
			break;
		case REFRESH:
			out << "REF";
			break;
		case DATA:
			out << "DATA"; 
			break;
		default:
			ERROR("Trying to print unknown kind of bus packet");
			exit(-1);
	}
	out << "] pa[0x"<<hex<<physicalAddress<<dec<<"] r["<<rank<<"] b["<<bank<<"] row["<<row<<"] col["<<column<<"]";
	if (busPacketType == DATA) {
		out << " ";
		printData(out);
	}
	return out;
}

ostream &BusPacket::printData(ostream &out) const 
{
	if (data == NULL) {
		out << "NO DATA";
		return out; 
	}

	out << "'" << hex;
	for (int i=0; i < 4; i++) {
		out << ((uint64_t *)data)[i];
	}
	out << dec << "'";
	return out; 
}

namespace DRAMSim {
	ostream &operator<<(ostream &out, const BusPacket &bp) {
		return bp.print(out);
	}
}
