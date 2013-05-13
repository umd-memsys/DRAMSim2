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
#include <string.h> //strndup
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
	operator T() {
		return value; 
	}
	
	ConfigOption<T> &operator=(const T &value_) {
		this->value = value_; 
		return *this; 
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
	if (value_str == "per_rank")
		value=PerRank; 
	else if (value_str == "per_rank_per_bank")
		value=PerRankPerBank; 
}

enum RowBufferPolicy
{
	OpenPage,
	ClosePage
};

template <>
inline void ConfigOption<RowBufferPolicy>::set(const std::string &value_str) {
	if (value_str == "close_page")
		value=ClosePage; 
	else if (value_str == "open_page")
		value=OpenPage; 
}

enum SchedulingPolicy
{
	RankThenBankRoundRobin,
	BankThenRankRoundRobin
};

template <>
inline void ConfigOption<SchedulingPolicy>::set(const std::string &value_str) {
	if (value_str == "rank_then_bank_round_robin")
		value=RankThenBankRoundRobin; 
	else if (value_str == "bank_then_rank_round_robin")
		value=BankThenRankRoundRobin; 
}

struct Config {

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
		// XXX: need some non-paramed initializer here to terminate the comma list 
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
	// disable copying 
	Config(const Config &other); 
	Config &operator=(const Config &other);
};
}

#endif // _INIREADER_H_
