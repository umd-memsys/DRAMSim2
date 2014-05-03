/*********************************************************************************
* Copyright (c) 2010-2011, 
* Jim Stevens, Paul Tschirhart, Ishwar Singh Bhati, Mu-Tien Chang, Peter Enns, 
* Elliott Cooper-Balis, Paul Rosenfeld, Bruce Jacob
* University of Maryland
* Contact: jims [at] cs [dot] umd [dot] edu
* All rights reserved.
*
* Redistribution and use in source and binary forms, with or without
* modification, are permitted provided that the following conditions are met:
*
* * Redistributions of source code must retain the above copyright notice,
* this list of conditions and the following disclaimer.
*
* * Redistributions in binary form must reproduce the above copyright notice,
* this list of conditions and the following disclaimer in the documentation
* and/or other materials provided with the distribution.
*
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
* ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
* WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
* DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
* FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
* DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
* SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
* CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
* OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
* OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*********************************************************************************/

#ifndef HYBRIDSYSTEM_UTIL_H
#define HYBRIDSYSTEM_UTIL_H

#include <stdlib.h>
#include <string>
#include <iostream>
#include <fstream>
#include <list>
#include <sstream>
#include <stdint.h>

using std::string;
using std::list; 
using std::cout; 

// Utility Library for HybridSim

template <typename T>
void convert(T &var, string value, string infostring = "") {
	// Check that each character in value is a digit.
	for(size_t i = 0; i < value.size(); i++)
	{
		if(!isdigit(value[i]))
		{
			cout << "ERROR: Non-digit character found: " << infostring << " : " << value << "\n";
			exit(-1); 
		}
	}

	// Convert it
	std::stringstream ss(value);
	ss >> var;
}

string strip(string input, string chars = " \t\f\v\n\r");
list<string> split(string input, string chars = " \t\f\v\n\r", size_t maxsplit=string::npos);

void confirm_directory_exists(string path);

#endif
