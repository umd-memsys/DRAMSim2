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

#include <iostream>
#include <fstream>
#include "PrintMacros.h"
#include "IniReader.h"

using namespace std;

namespace DRAMSim
{

OptionsMap IniReader::ReadIniFile(string filename)
{
	OptionsMap options; 
	ifstream iniFile;
	string line;
	string key,valueString;

	size_t commentIndex, equalsIndex;
	size_t lineNumber=0;

	iniFile.open(filename.c_str());
	if (iniFile.is_open())
	{
		std::cerr << "Reading ini file '"<<filename<<"'\n";
		while (!iniFile.eof())
		{
			lineNumber++;
			getline(iniFile, line);
			//this can happen if the filename is actually a directory
			if (iniFile.bad())
			{
				ERROR("Cannot read ini file '"<<filename<<"'");
				abort();
			}
			// skip zero-length lines
			if (line.size() == 0)
			{
//					DEBUG("Skipping blank line "<<lineNumber);
				continue;
			}
			//search for a comment char
			if ((commentIndex = line.find_first_of(";")) != string::npos)
			{
				//if the comment char is the first char, ignore the whole line
				if (commentIndex == 0)
				{
//						DEBUG("Skipping comment line "<<lineNumber);
					continue;
				}
//					DEBUG("Truncating line at comment"<<line[commentIndex-1]);
				//truncate the line at first comment before going on
				line = line.substr(0,commentIndex);
			}
			// trim off the end spaces that might have been between the value and comment char
			size_t whiteSpaceEndIndex;
			if ((whiteSpaceEndIndex = line.find_last_not_of(" \t")) != string::npos)
			{
				line = line.substr(0,whiteSpaceEndIndex+1);
			}

			// at this point line should be a valid, commentless string

			// a line has to have an equals sign
			if ((equalsIndex = line.find_first_of("=")) == string::npos)
			{
				ERROR("Malformed Line "<<lineNumber<<" (missing equals)");
				abort();
			}
			size_t strlen = line.size();
			// all characters before the equals are the key
			key = line.substr(0, equalsIndex);
			// all characters after the equals are the value
			valueString = line.substr(equalsIndex+1,strlen-equalsIndex);
			options[key] = valueString; 

		}
	}
	else
	{
		ERROR ("Unable to load ini file "<<filename);
		abort();
	}
	return options; 
}

} // namespace DRAMSim
