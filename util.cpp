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

#include <stdlib.h>
#include "util.h"
using namespace std; 


string strip(string input, string chars)
{
	size_t pos;

	// Strip front.
	pos = input.find_first_not_of(chars);
	input.erase(0, pos);

	// Strip back.
	pos = input.find_last_not_of(chars);
	input.erase(pos+1);

	return input;
}


list<string> split(string input, string chars, size_t maxsplit)
{
	list<string> ret;

	string cur = input;

	size_t pos = 0;

	if (maxsplit == 0)
		maxsplit = 1;

	while (!cur.empty())
	{
		if (ret.size() == (maxsplit-1))
		{
			ret.push_back(cur);
			return ret;
		}

		pos = cur.find_first_of(chars);
		string tmp = cur.substr(0, pos);
		ret.push_back(tmp);


		if (pos == string::npos)
			cur.erase();
		else
		{
			// Skip ahead to the next non-split char
			size_t new_pos = cur.find_first_not_of(chars, pos);
			//cur.erase(0, pos+1);
			cur.erase(0, new_pos);
		}
	}

	// If not at npos, then there is still an extra empty string at the end.
	if (pos != string::npos)
	{
		ret.push_back("");
	}

	return ret;
}

void confirm_directory_exists(string path)
{
	string command_str = "test -e "+path+" || mkdir "+path;
	const char * command = command_str.c_str();
	int sys_done = system(command);
	if (sys_done != 0)
	{
		cout << "system command to confirm directory "+path+" exists has failed.";
		abort();
	}
}
