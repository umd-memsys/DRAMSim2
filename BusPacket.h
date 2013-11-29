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

#ifndef _BUSPACKET_H_
#define _BUSPACKET_H_

#include <vector>
//FIXME: move code out of header and forward delcare this instead  
#include "ConfigIniReader.h"
#include "AddressMapping.h"

using std::vector;
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

	class Config; 
	class Transaction;
	class BusPacket
	{
		BusPacket();

		vector<BusPacket *> dependencies;
		vector<BusPacket *> notifyList;
		public:
		//Fields
		BusPacketType busPacketType;
		uint64_t address;
		unsigned column;
		unsigned row;
		unsigned bank;
		unsigned rank;
		uint64_t physicalAddress;
		void *data;
		Transaction *sourceTransaction;

		unsigned globalBankId; 
		unsigned globalRowId; 


		public:

		//Functions
		BusPacket(BusPacketType packtype, uint64_t physicalAddr, unsigned col, unsigned rw, unsigned r, unsigned b, const Config &cfg, void *dat);

		void setSourceTransaction(Transaction *t) {
			sourceTransaction = t; 
		}

		inline bool hasDependencies() {
			return !dependencies.empty();
		}

		bool isDependent(BusPacket *other) const {
			// TODO: check ACTIVATE; currently, ACTIVATE can be sent with a refresh(?)
			// before checking banks/rows, make sure every packet is dependent on a refresh 
			if (other->busPacketType == REFRESH && this->rank == other->rank) {
				return true; 
			}

			// otherwise, the most likely case is they simply aren't going to the same bank
			if (this->globalBankId != other->globalBankId) {
				return false;
			}

			// all requests to same row must be serialized
			if (this->globalRowId == other->globalRowId) {
				return true; 
			}

			// TODO: any other cases: going to same bank, but different rows
			return false; 
		}

		void clearDependency(const BusPacket *dependentBP) {
			// should only be one dep in our list, so might be enough to just return after the erase()
			for (vector<BusPacket *>::iterator it = dependencies.begin(); it != dependencies.end(); ) {
				if (*it == dependentBP) {
					it = dependencies.erase(it);
				} else {
					++it;
				}
			}
		}

		void notifyAllDependents() {
			for (size_t i=0; i<notifyList.size(); ++i) {
				notifyList[i]->clearDependency(this);
			}
			notifyList.clear();
		}

		void addNotifyList(BusPacket *reverseDependency) {
			notifyList.push_back(reverseDependency);
		}

		void setDependsOn(BusPacket *dependentBP) {
			this->dependencies.push_back(dependentBP); 	
			dependentBP->addNotifyList(this);
		}

		ostream &print(ostream &out) const;
		void print(uint64_t currentClockCycle, bool dataStart);
		ostream &printData(ostream &out) const;
	};
	ostream &operator<<(ostream &out, const BusPacket &bp);
}

#endif

