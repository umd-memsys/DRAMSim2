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



#ifndef SYSCONFIG_H
#define SYSCONFIG_H

#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <cstdlib>
#include <stdint.h>
#include "PrintMacros.h"

#ifdef __APPLE__
#include <sys/types.h>
#endif

//SystemConfiguration.h
//
//Configuration values for the current system

extern std::ofstream cmd_verify_out; //used by BusPacket.cpp if VERIFICATION_OUTPUT is enabled

namespace DRAMSim
{

//
//FUNCTIONS
//

unsigned inline dramsim_log2(unsigned value)
{
	unsigned logbase2 = 0;
	unsigned orig = value;
	value>>=1;
	while (value>0)
	{
		value >>= 1;
		logbase2++;
	}
	if (1U<<logbase2 < orig)
		logbase2++;
	return logbase2;
}

inline bool isPowerOfTwo(unsigned long x)
{
	return (1UL<<dramsim_log2(x)) == x;
}


};

#endif

