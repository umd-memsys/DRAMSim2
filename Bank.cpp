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






//Bank.cpp
//
//Class file for bank object
//

#include "Bank.h"
#include "BusPacket.h"

using namespace std;
using namespace DRAMSim;

Bank::Bank():
		rowEntries(NUM_COLS)
{}

/* The bank class is just a glorified sparse storage data structure
 * that keeps track of written data in case the simulator wants a
 * function DRAM model
 *
 * A vector of size NUM_COLS keeps a linked list of rows and their
 * associated values.
 *
 * write() adds an entry to the proper linked list or replaces the
 * 	value in a row that was already written
 *
 * read() searches for a node with the right row value, if not found
 * 	returns the tracer value 0xDEADBEEF
 * 
 *	TODO: if anyone wants to actually store data, this should be changed to an STL unordered_map 
 */



Bank::DataStruct *Bank::searchForRow(uint row, DataStruct *head)
{
	while (head != NULL)
	{
		if (head->row == row)
		{
			//found it
			return head;
		}
		//keep looking
		head = head->next;
	}
	//if we get here, didn't find it
	return NULL;
}

void Bank::read(BusPacket *busPacket)
{
	DataStruct *rowHeadNode = rowEntries[busPacket->column];
	DataStruct *foundNode = NULL;

	if ((foundNode = Bank::searchForRow(busPacket->row, rowHeadNode)) == NULL)
	{
		// the row hasn't been written before, so it isn't in the list
		//if(SHOW_SIM_OUTPUT) DEBUG("== Warning - Read from previously unwritten row " << busPacket->row);
		void *garbage = calloc(BL * JEDEC_DATA_BUS_WIDTH,1);
		((long *)garbage)[0] = 0xdeadbeef; // tracer value
		busPacket->data = garbage;
	}
	else // found it
	{
		busPacket->data = foundNode->data;
	}

	//the return packet should be a data packet, not a read packet
	busPacket->busPacketType = DATA;
}


void Bank::write(const BusPacket *busPacket)
{
	//TODO: move all the error checking to BusPacket so once we have a bus packet,
	//			we know the fields are all legal

	if (busPacket->column >= NUM_COLS)
	{
		ERROR("== Error - Bus Packet column "<< busPacket->column <<" out of bounds");
		exit(-1);
	}

	// head of the list we need to search
	DataStruct *rowHeadNode = rowEntries[busPacket->column];
	DataStruct *foundNode = NULL;

	if ((foundNode = Bank::searchForRow(busPacket->row, rowHeadNode)) == NULL)
	{
		//not found
		DataStruct *newRowNode = (DataStruct *)malloc(sizeof(DataStruct));

		//insert at the head for speed
		//TODO: Optimize this data structure for speedier lookups?
		newRowNode->row = busPacket->row;
		newRowNode->data = busPacket->data;
		newRowNode->next = rowHeadNode;
		rowEntries[busPacket->column] = newRowNode;
	}
	else
	{
		// found it, just plaster in the new data
		foundNode->data = busPacket->data;
		if (DEBUG_BANKS)
		{
			PRINTN(" -- Bank "<<busPacket->bank<<" writing to physical address 0x" << hex << busPacket->physicalAddress<<dec<<":");
			BusPacket::printData(busPacket->data);
			PRINT("");
		}
	}
	delete(busPacket);
}

