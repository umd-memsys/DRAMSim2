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





#include "dramsim_test.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

using namespace DRAMSim;

/* callback functors */
void some_object::read_complete(uint64_t address, void *data, size_t bufsize)
{
	printf("[Callback] read complete: 0x%lx data='", address);
	for (size_t i=0; i<bufsize; i++)
	{
		printf("%02x ",((unsigned char*)data)[i]);
	}
	printf("'\n"); 
}

void some_object::write_complete(unsigned id, uint64_t address, uint64_t clock_cycle)
{
	printf("[Callback] write complete: %d 0x%lx cycle=%lu\n", id, address, clock_cycle);
}

/* FIXME: this may be broken, currently */
void power_callback(double a, double b, double c, double d)
{
	printf("power callback: %0.3f, %0.3f, %0.3f, %0.3f\n",a,b,c,d);
}

int some_object::add_one_and_run()
{
	/* pick a DRAM part to simulate */
	MemorySystem *mem = getMemorySystemInstance(0, "ini/DDR2_micron_16M_8b_x8_sg3E.ini", "system.ini", "..", "resultsfilename", 2048); 

	/* create and register our callback functions */
	ReadDataCB *read_cb = 
		new Callback<some_object, void, uint64_t, void *, size_t>(this, &some_object::read_complete);
	TransactionCompleteCB *write_cb = 
		new Callback<some_object, void, unsigned, uint64_t, uint64_t>(this, &some_object::write_complete);
	mem->RegisterCallbacks(read_cb, write_cb, power_callback);


	void *write_buf = malloc(32);
	memset(write_buf, 1, 32); 
	mem->addTransaction(true, 0x9000, write_buf, 32);

	for (int i=0; i<5; i++)
	{
		mem->update();
	}

	// read back the data that was just written
	mem->addTransaction(false, 0x9000, NULL, 0);

	for (int i=0; i<5; i++)
	{
		mem->update();
	}

	write_buf = malloc(8);
	memset(write_buf, 0xa5, 8);
	mem->addTransaction(true, 0x9010, write_buf, 8);

	for (int i=0; i<5; i++)
	{
		mem->update();
	}


	mem->addTransaction(false, 0x9000, NULL, 0);

	for (int i=0; i<45; i++)
	{
		mem->update();
	}

	/* get a nice summary of this epoch */
	mem->printStats();

	return 0;
}

int main()
{
	printf("dramsim_test main()\n");
	some_object obj;
	obj.add_one_and_run();
}

