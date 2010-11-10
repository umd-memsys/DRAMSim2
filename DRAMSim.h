/****************************************************************************
*	 DRAMSim2: A Cycle Accurate DRAM simulator 
*	 
*	 Copyright (C) 2010   	Elliott Cooper-Balis
*									Paul Rosenfeld 
*									Bruce Jacob
*									University of Maryland
*
*	 This program is free software: you can redistribute it and/or modify
*	 it under the terms of the GNU General Public License as published by
*	 the Free Software Foundation, either version 3 of the License, or
*	 (at your option) any later version.
*
*	 This program is distributed in the hope that it will be useful,
*	 but WITHOUT ANY WARRANTY; without even the implied warranty of
*	 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*	 GNU General Public License for more details.
*
*	 You should have received a copy of the GNU General Public License
*	 along with this program.  If not, see <http://www.gnu.org/licenses/>.
*
*****************************************************************************/


#ifndef DRAMSIM_H
#define DRAMSIM_H
/*
 * This is a public header for DRAMSim including this along with libdramsim.so should
 * provide all necessary functionality to talk to an external simulator
 */
#include "Callback.h"
using std::string;

namespace DRAMSim 
{
	class MemorySystem
	{
		public:
			bool addTransaction(bool isWrite, uint64_t addr);
			bool WillAcceptTransaction();
			void update();
			void printStats();
			void RegisterCallbacks( 
				TransactionCompleteCB *readDone,
				TransactionCompleteCB *writeDone,
				void (*reportPower)(double bgpower, double burstpower, double refreshpower, double actprepower));
	};
	MemorySystem *getMemorySystemInstance(uint id, string dev, string sys, string pwd, string trc);
}


#endif
