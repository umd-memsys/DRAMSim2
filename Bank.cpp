/*********************************************************************************
*  Copyright (c) 2010-2011, Elliott Cooper-Balis
*                             Paul Rosenfeld
*                             Bruce Jacob
*                             University of Maryland 
*                             dramninjas [at] umd [dot] edu
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








//Bank.cpp
//
//Class file for bank object
//

#include "Bank.h"
#include "BusPacket.h"
#include <string.h> // for memcpy
#include <assert.h>

using namespace std;
using namespace DRAMSim;

Bank::Bank()
{}

unsigned Bank::getByteOffsetInRow(const BusPacket *busPacket)
{
	// This offset will simply be all of the bits in the unaligned address that
	// have been removed (i.e. the lower 6 bits for BL=8 and the lower 5 bits
	// for BL=4) plus the column offset
	
	unsigned transactionSize = (JEDEC_DATA_BUS_BITS/8)*BL; 
	uint64_t transactionMask =  transactionSize - 1; //ex: (64 bit bus width) x (8 Burst Length) - 1 = 64 bytes - 1 = 63 = 0x3f mask
	unsigned byteOffset = busPacket->data->getAddr() & transactionMask;
	unsigned columnOffset = (busPacket->column * DEVICE_WIDTH)/8; 
	unsigned offset = columnOffset + byteOffset; 
	DEBUG("[DPKT] "<< *(busPacket->data) << " \t r="<<busPacket->row<<" c="<<busPacket->column<<" byte offset="<< byteOffset<< "-> "<<offset);  
	return offset; 
}
void Bank::write(const BusPacket *busPacket)
{
	unsigned byteOffset = getByteOffsetInRow(busPacket);
	row_map_t::iterator it; 
	it = rowEntries.find(busPacket->row);
	byte *rowData;
	if (it == rowEntries.end()) 
	{
		// row doesn't exist yet, allocate it
		rowData = (byte *)calloc((NUM_COLS*DEVICE_WIDTH)/8,sizeof(byte)); 
		rowEntries[busPacket->row] = rowData;
	}
	else
	{
		rowData = it->second; 
	}

	// if we out of bound a row, this is a problem
	if (byteOffset + busPacket->data->getNumBytes() > NUM_COLS*DEVICE_WIDTH)
	{
		ERROR("Transaction out of bounds a row, check alignment of the address"); 
		exit(-1); 
	}
	// copy data to the row 
	memcpy(rowData + byteOffset, busPacket->data->getData(), busPacket->data->getNumBytes());
}


void Bank::read(BusPacket *busPacket)
{

	assert(busPacket->data == NULL); 
	row_map_t::iterator it; 
	it = rowEntries.find(busPacket->row);
	unsigned transactionSize = (JEDEC_DATA_BUS_BITS/8)*BL;
	byte *dataBuf = (byte *)calloc(sizeof(byte), transactionSize);
	if (it != rowEntries.end())
	{
		byte *rowData = it->second; 
		memcpy(dataBuf, rowData + (busPacket->column*DEVICE_WIDTH)/8, transactionSize);
	}

	busPacket->data = new DataPacket(dataBuf, transactionSize, busPacket->physicalAddress);
	DEBUG("[DPKT] Rank returning: "<<*(busPacket->data));
	busPacket->busPacketType = DATA;

}

