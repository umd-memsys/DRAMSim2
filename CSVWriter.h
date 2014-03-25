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
#ifndef _CSV_WRITER_H_
#define _CSV_WRITER_H_

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <iostream>
#include <sstream>
#include <vector>
#include <string>
#include <fstream>

#include <string.h>

using std::vector; 
using std::ostream;
using std::ofstream;
using std::string; 
using std::stringstream;
/*
 * CSVWriter: Writes CSV data with headers to an underlying ostream 
 * 	This wrapper is meant to look like an ostream, but it captures 
 * 	the names of each field and prints it out to a header before printing
 * 	the CSV data below. 
 *
 * 	Note: the first finalize() will not print the values out, only the headers.
 * 	One way to fix this problem would be to use a sstringstream (or something) 
 * 	to buffer out the values and flush them all in one go instead of passing them 
 * 	directly to the underlying stream as is the case now. 
 *
 * 	Example usage: 
 *
 * 	CSVWriter sw(cout);               // send output to cout
 * 	sw <<"Bandwidth" << 0.5; // value ignored
 * 	sw <<"Latency" << 5;     // value ignored
 * 	sw.finalize();                      // flush the header 
 * 	sw <<"Bandwidth" << 1.5; // field name ignored
 * 	sw <<"Latency" << 15;     // field name ignored
 * 	sw.finalize(); 							// values printed to csv line
 * 	sw <<"Bandwidth" << 2.5; // field name ignored
 * 	sw <<"Latency" << 25;     // field name ignored
 * 	sw.finalize(); 							// values printed to csv line
 *
 * 	The output of this example will be: 
 *
 * 	Bandwidth,Latency
 * 	1.5,15
 * 	2.5,25
 *
 */


namespace DRAMSim {

	class CSVWriter {
		public :
		struct IndexedName {
			static const size_t MAX_TMP_STR = 64; 
			static const unsigned SINGLE_INDEX_LEN = 4; 
			string str; 

			// functions 
			static bool isNameTooLong(const char *baseName, unsigned numIndices)
			{
				return (strlen(baseName)+(numIndices*SINGLE_INDEX_LEN)) > MAX_TMP_STR;
			}
			static void checkNameLength(const char *baseName, unsigned numIndices)
			{
				if (isNameTooLong(baseName, numIndices))
				{
					std::cerr << "Your string "<<baseName<<" is too long for the max stats size ("<<MAX_TMP_STR<<", increase MAX_TMP_STR\n";
					exit(-1); 
				}
			}
			IndexedName(const char *baseName, const string &suffix) {
				stringstream ss; 
				ss << baseName << suffix;
				str = ss.str(); 
			}
			IndexedName(const char *baseName, unsigned channel)
			{
				checkNameLength(baseName,1);
				char tmp_str[MAX_TMP_STR]; 
				snprintf(tmp_str, MAX_TMP_STR,"%s[%u]", baseName, channel); 
				str = string(tmp_str); 
			}
			IndexedName(const char *baseName, unsigned channel, unsigned rank)
			{
				checkNameLength(baseName,2);
				char tmp_str[MAX_TMP_STR]; 
				snprintf(tmp_str, MAX_TMP_STR,"%s[%u][%u]", baseName, channel, rank); 
				str = string(tmp_str); 
			}
			IndexedName(const char *baseName, unsigned channel, unsigned rank, unsigned bank)
			{
				checkNameLength(baseName,3);
				char tmp_str[MAX_TMP_STR]; 
				snprintf(tmp_str, MAX_TMP_STR,"%s[%u][%u][%u]", baseName, channel, rank, bank); 
				str = string(tmp_str);
			}

		};
		struct StringWrapper {
			string s;
			StringWrapper(const string &_s) : s(_s)
			{}
		};
		// where the output will eventually go 
		ostream &output; 
		vector<string> fieldNames; 
		bool finalized; 
		unsigned idx; 
		// This is a poor man's parity -- tag each known value (like int, float, etc) and then we know that a field name has to follow a real value, and if a string follows a field, it is actually a value. Might be able to just use the parity of idx to do this, but this seems to be a little safer. 
		bool lastFieldWasValue;
		string firstField;
		public: 

		~CSVWriter() {
			endLine();
			output.flush();
		}

		void printHeader() {
				for (unsigned i=0; i<fieldNames.size(); i++)
				{
					output << fieldNames[i] << ",";
					//std::cout <<i<<": '"<<fieldNames[i]<<"'\n";
				}
				finalized=true; 
				endLine();
		}

		void endLine() {
			output << std::endl << std::flush;
			idx=0;
		}

		// Functions
		void finalize()
		{
			// now a no-op 
		}


		// Constructor 
		CSVWriter(ostream &_output) : output(_output), finalized(false), idx(0), lastFieldWasValue(true)
		{}

		bool isFinalized() {
			return finalized; 
		}
		// Insertion operators for field names
		CSVWriter &operator<<(const char *name)
		{
			//std::cout <<"Got string '"<<name<<"' finalized? "<<finalized<<" lastFieldValue?"<<lastFieldWasValue<<std::endl;
			if (!finalized)
			{
				if (lastFieldWasValue) {
					if (fieldNames.empty()) {
						firstField = string(name);
						std::cout << "First field is "<< firstField<<"\n";
						fieldNames.push_back(string(name));
					}
					else {
						if (firstField == name) {
							finalized = true;
							printHeader();
						} 
						else {
							fieldNames.push_back(string(name));
						}
					}
				}
			}
			else // finalized
			{
				if (!lastFieldWasValue) {
					output << name << ",";
					idx++;
				}
				if (idx == fieldNames.size()) {
					endLine();
				}

			}
			lastFieldWasValue = !lastFieldWasValue;
			return *this; 
		}

		CSVWriter &operator<<(const string &name)
		{
			return (*this)<<name.c_str();
		}
		
		CSVWriter &operator<<(const IndexedName &indexedName)
		{
			return (*this)<<indexedName.str.c_str();
		}

		ostream &getOutputStream()
		{
			return output; 
		}
		// Insertion operators for value types 
		// All of the other types just need to pass through to the underlying
		// ofstream, so just write this small wrapper function to make the
		// whole thing less verbose
#define ADD_TYPE(T) \
		CSVWriter &operator<<(T value)            \
		{                                         \
			if (finalized)                         \
			{                                      \
				output << value <<",";              \
				idx++;                              \
				if (idx == fieldNames.size()) {     \
					endLine();                       \
				}                                   \
			}                                      \
			lastFieldWasValue=true;                \
			return *this;                          \
		}                      

	ADD_TYPE(int);
	ADD_TYPE(unsigned); 
	ADD_TYPE(long);
	ADD_TYPE(uint64_t);
	ADD_TYPE(float);
	ADD_TYPE(double);

	static CSVWriter &GetCSVWriterInstance(const string &filename) {
		std::cerr<<"Creating new CSVWriter '"<< filename<<"'\n";
		ostream &output = *(new ofstream(filename.c_str())); 
		return *(new CSVWriter(output)); 
	}

	//disable copy constructor and assignment operator
	private:
		CSVWriter(const CSVWriter &); 
		CSVWriter &operator=(const CSVWriter &);
		
	}; // class CSVWriter


} // namespace DRAMSim

#endif // _CSV_WRITER_H_
