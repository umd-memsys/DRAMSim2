/*********************************************************************************
*  Copyright (c) 2010-2013, Paul Rosenfeld
*                            Elliott Cooper-Balis
*                            Bruce Jacob
*                            University of Maryland 
*                            dramninjas [at] gmail [dot] com
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

#ifndef _CONFIGINIREADER_H_
#define _CONFIGINIREADER_H_

#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <iostream>
#include <string>
//#include <CSVWriter.h>
#include <sstream> 
#include "DRAMSim.h"


using namespace std; 

namespace DRAMSim {

template <typename T>
class ConfigOption {
	std::string optionName;
	T value;
	std::string optionValueString; 

	// has this option been overridden from the default value? 
	bool setAfterDefault; 
	public:
	ConfigOption(const std::string &name_, const T &value_) : 
		optionName(name_),
		value(T(value_)),
		setAfterDefault(false)
	{}

	void set(const T &value_) {
		value=T(value_);
		setAfterDefault=true; 
	}

	void set(const std::string &value_str) {
		this->optionValueString = value_str; 
		std::istringstream iss(value_str);
		iss >> std::boolalpha >> this->value; 
		setAfterDefault = true; 
	}
	const std::string &getName() const {
		return optionName; 
	}

	/* this allows for conversion back to the primitive type such as 
	 * float tCK = cfg.tCK
	 * 	or 
	 * if (cfg.tCK > 5.0f) {
	 *  foo();
	 * }
	 */
	operator T() const {
		return value; 
	}

	/* this allows assignment from the primitive type to the option, such as: 
	 * cfg.tCK=5.0; 
	 */
	ConfigOption<T> &operator=(const T &value_) {
		this->value = value_; 
		return *this; 
	}

	ostream &print(ostream &out) {
		if (setAfterDefault) {
			out << "[*] ";
		}
		else {
			out << "    ";
		}
		out << getName() << "=" << value;
		return out; 
	}

};


enum AddressMappingScheme
{
	Scheme1,
	Scheme2,
	Scheme3,
	Scheme4,
	Scheme5,
	Scheme6,
	Scheme7
};

template <>
inline void ConfigOption<AddressMappingScheme>::set(const std::string &value_str) {
	if (value_str == "Scheme1")
		value=Scheme1; 
	else if (value_str == "Scheme2")
		value=Scheme2; 
	else if (value_str == "Scheme3")
		value=Scheme3; 
	else if (value_str == "Scheme4")
		value=Scheme4; 
	else if (value_str == "Scheme5")
		value=Scheme5; 
	else if (value_str == "Scheme6")
		value=Scheme6; 
	else if (value_str == "Scheme7")
		value=Scheme7; 
	else 
		std::cerr << "WARNING: Unknown value '"<< value_str<<"' for option "<<getName()<<", using default\n";
}

// Template specialization for enums setters from strings 

// Only used in CommandQueue
enum QueuingStructure
{
	PerRank,
	PerRankPerBank
};

template <>
inline void ConfigOption<QueuingStructure>::set(const std::string &value_str) {
	if (value_str == "PerRank")
		value=PerRank; 
	else if (value_str == "PerRankPerBank")
		value=PerRankPerBank; 
	else 
		std::cerr << "WARNING: Unknown value '"<< value_str<<"' for option "<<getName()<<", using default\n";
}

enum RowBufferPolicy
{
	OpenPage,
	ClosePage
};

template <>
inline void ConfigOption<RowBufferPolicy>::set(const std::string &value_str) {
	if (value_str == "ClosePage")
		value=ClosePage; 
	else if (value_str == "OpenPage")
		value=OpenPage; 
	else 
		std::cerr << "WARNING: Unknown value '"<< value_str<<"' for option "<<getName()<<", using default\n";
}

enum SchedulingPolicy
{
	RankThenBankRoundRobin,
	BankThenRankRoundRobin
};

template <>
inline void ConfigOption<SchedulingPolicy>::set(const std::string &value_str) {
	if (value_str == "RankThenBankRoundRobin")
		value=RankThenBankRoundRobin; 
	else if (value_str == "BankThenRankRoundRobin")
		value=BankThenRankRoundRobin; 
	else 
		std::cerr << "WARNING: Unknown value '"<< value_str<<"' for option "<<getName()<<", using default\n";
}

class Config {
	public: 

	/* Generate the member definitions */
#define PARAM(type, name, default_value) \
	ConfigOption<type> name; 
#include "params.def"
#undef PARAM

	bool finalized; 
	Config() : 
	/* Generate the initializer list */ 
#define PARAM(type, name, default_value) \
		name(#name, default_value),
#include "params.def"
#undef PARAM
		// XXX: need some non-param'd initializer here to terminate the comma list (i.e., do not remove the next line)
		finalized(false)
	{}

	OptionsFailedToSet set(const OptionsMap &options) {
		OptionsFailedToSet failures; 
		for(OptionsMap::const_iterator it = options.begin(); it != options.end(); ++it){
			if (!this->set(it->first, it->second)) {
				failures.push_back(it->first); 
			}
		}
		return failures; 
	}
	ostream &print(ostream &out) {
		out << " ==== Configuration parameters; [*] indicates a change from default ==== \n";

#define PARAM(type, name,default_value) \
		name.print(out); \
		out << " (default="<< #default_value<<")\n";
#include "params.def"
#undef PARAM
		return out; 
	}

	private:
	bool set(const std::string &key, const std::string &value) {
#define PARAM(type, name, default_value) \
		if (name.getName() == key) { \
			name.set(value); \
			return true; \
		}
#include "params.def"
#undef PARAM

		return false;
	}
	public:
	/**
	 * This function fixes up the number of ranks based on megs of memory and the device density. 
	 *
	 * If the number of ranks is already set and it will divide evenly among the channels, it will be left alone
	 * If the number of ranks cannot be split among the channels, it will try to set NUM_RANKS based on megsOfMemory
	 * If megsOfMemory cannot be split among single-rank channels, the smallest memory system size that can be made of single rank channels is created
	 *
	 * XXX: important note -- 
	 * 		the simulator used to only be able to simulate one channel so NUM_RANKS really means "number of ranks per channel"
	 * 		however, due to the naming, it makes more sense to have the user put the *total* number of ranks into the ini file
	 * 		so this finalize() function will split up the total number of ranks specified in the ini file among the channels
	 * 		and after it is run, NUM_RANKS will mean "number of ranks per channel"
	 */
	void finalize() {

		//calculate number of devices
		/******************************************************************
		  This code has always been problematic even though it's pretty simple. I'll try to explain it 
		  for my own sanity. 

		  There are two main variables here that we could let the user choose:
		  NUM_RANKS or TOTAL_STORAGE.  Since the density and width of the part is
		  fixed by the device ini file, the only variable that is really
		  controllable is the number of ranks. Users care more about choosing the
		  total amount of storage, but with a fixed device they might choose a total
		  storage that isn't possible. In that sense it's not as good to allow them
		  to choose TOTAL_STORAGE (because any NUM_RANKS value >1 will be valid).

		  However, users don't care (or know) about ranks, they care about total
		  storage, so maybe it's better to let them choose and just throw an error
		  if they choose something invalid. 

		  A bit of background: 

		  Each column contains DEVICE_WIDTH bits. A row contains NUM_COLS columns.
		  Each bank contains NUM_ROWS rows. Therefore, the total storage per DRAM device is: 
		  PER_DEVICE_STORAGE = NUM_ROWS*NUM_COLS*DEVICE_WIDTH*NUM_BANKS (in bits)

		  A rank *must* have a 64 bit output bus (JEDEC standard), so each rank must have:
		  NUM_DEVICES_PER_RANK = 64/DEVICE_WIDTH  
		  (note: if you have multiple channels ganged together, the bus width is 
		  effectively NUM_CHANS * 64/DEVICE_WIDTH)

		  If we multiply these two numbers to get the storage per rank (in bits), we get:
		  PER_RANK_STORAGE = PER_DEVICE_STORAGE*NUM_DEVICES_PER_RANK = NUM_ROWS*NUM_COLS*NUM_BANKS*64 

		  Finally, to get TOTAL_STORAGE, we need to multiply by NUM_RANKS
		  TOTAL_STORAGE = PER_RANK_STORAGE*NUM_RANKS (total storage in bits)

		  So one could compute this in reverse -- compute NUM_DEVICES,
		  PER_DEVICE_STORAGE, and PER_RANK_STORAGE first since all these parameters
		  are set by the device ini. Then, TOTAL_STORAGE/PER_RANK_STORAGE = NUM_RANKS 

		  The only way this could run into problems is if TOTAL_STORAGE < PER_RANK_STORAGE,
		  which could happen for very dense parts.
		 *********************/

		// number of bytes per rank
		unsigned long megsOfStoragePerRank = ((((long long)NUM_ROWS * (NUM_COLS * DEVICE_WIDTH) * NUM_BANKS) * ((long long)JEDEC_DATA_BUS_BITS / DEVICE_WIDTH)) / 8UL) >> 20UL;


		// first, split the total number of ranks among the channels. this *can* be zero 
		this->NUM_RANKS = this->NUM_RANKS / this->NUM_CHANS;

		if (this->NUM_RANKS == 0) {
			this->NUM_RANKS = this->megsOfMemory / (this->NUM_CHANS * megsOfStoragePerRank);
			if (this->NUM_RANKS == 0)
			{
				std::cerr<<"WARNING: Cannot create memory system with "<<this->megsOfMemory<<"MB, defaulting to minimum rank size of "<<megsOfStoragePerRank<<"MB (total size="<<this->NUM_CHANS * megsOfStoragePerRank<<"MB) \n";
				this->NUM_RANKS = 1;
			}
		}
		// at this point, NUM_RANKS *must* be set to some valid value, so fixup the megsOfMemory based on the number of ranks  

		this->megsOfMemory = this->NUM_RANKS * megsOfStoragePerRank;

		// Fix up all the other formulas from the def file 
		RL = CL + AL;
		WL = RL - 1;
		READ_TO_PRE_DELAY     = (AL+BL/2+max((unsigned)tRTP,2U)-2);
		WRITE_TO_PRE_DELAY    = (WL+BL/2+tWR);
		READ_TO_WRITE_DELAY   = (RL+BL/2+tRTRS-WL);
		READ_AUTOPRE_DELAY    = (AL+tRTP+tRP);
		WRITE_AUTOPRE_DELAY   = (WL+BL/2+tWR+tRP);
		WRITE_TO_READ_DELAY_B = (WL+BL/2+tWTR);
		WRITE_TO_READ_DELAY_R = (WL+BL/2+tRTRS-RL);
		NUM_DEVICES = (JEDEC_DATA_BUS_BITS/DEVICE_WIDTH);


		finalized=true; 

	}
	private:
	// disable copying 
	Config(const Config &other); 
	Config &operator=(const Config &other);
};
} // namespace 

#endif // _CONFIGINIREADER_H_
