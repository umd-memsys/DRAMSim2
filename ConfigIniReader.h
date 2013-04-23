#ifndef _INIREADER_H_
#define _INIREADER_H_

#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h> //strndup
#include <iostream>
#include <map>
#include <string>
//#include <CSVWriter.h>
#include <sstream> 


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
		iss >> this->value; 
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

};


enum RowBufferPolicy
{
	OpenPage,
	ClosePage
};

// Template specialization for enums setters from strings 
template <>
void ConfigOption<RowBufferPolicy>::set(const std::string &value_str) {
	if (value_str == "ClosePage")
		value=ClosePage; 
	else if (value_str == "OpenPage")
		value=OpenPage; 
}


struct Config {
	typedef std::map<std::string, std::string> OptionsMap;
	typedef std::map<std::string, bool> OptionsSuccessfullySetMap; 

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

	OptionsSuccessfullySetMap set(const OptionsMap &options) {
		OptionsSuccessfullySetMap successes; 
		for(OptionsMap::const_iterator it = options.begin(); it != options.end(); ++it){
			successes[it->first] = this->set(it->first, it->second);
		}
		return successes; 
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
};

	

#if 0 

	public:
OptionMap optionMap; 
const AddressMappingScheme mappingScheme;
const RefreshScheme refreshScheme;

//generate the actual members of class Config
		#define PARAM(type, var_name, default_val) \
			type var_name;
		#include "config.def"

	Config() : 
		mappingScheme(RW_CL_BK_RK_BY)
		, refreshScheme(AllBanksInPart)
// generate the initializer list for the members of config
#define PARAM(type, var_name, default_val) \
		, var_name(default_val)
		#include "config.def"
	{
	// generate the optionMap
#define PARAM(type, var_name, default_val) \
		optionMap[string(#var_name)] = 0;
#include "config.def"
	}
	// public functions
	void print(ostream &os, bool onlyNonDefault=false) const {
		OptionMap::const_iterator it; 
		#define PARAM(type, var_name, default_val) \
		if ((it=optionMap.find(string(#var_name))) != optionMap.end()) { \
			string updateString("");   \
			bool wasUpdated=false;		\
			if ((wasUpdated = (it->second > 0))) {       \
				updateString="[*] ";			\
			}                           \
			if ((onlyNonDefault && wasUpdated) || (!onlyNonDefault)) { \
			os << updateString<<"["<<#type<<"]: '"<<#var_name<<"'="<<var_name<<" [def: "<<#default_val<<"]\n"; \
			} \
		}
		#include "config.def"
	}
	void dumpStats(CSVWriter &CSVOut, bool dumpOnlyUpdated=true) const {
		OptionMap::const_iterator it; 
#define PARAM(type, var_name, default_val) \
			if ((it=optionMap.find(string(#var_name))) != optionMap.end()) { \
				if (!dumpOnlyUpdated || (dumpOnlyUpdated && it->second > 0)) { \
					string key("m:"); \
					key += #var_name; \
					CSVOut << key << var_name; \
					/*cerr << key<<" -> "<<var_name<<endl;*/ \
				} \
			}
			#include "config.def"
	}
	bool hasOption(const string &s) const 
	{
		return (optionMap.count(s) > 0);
	}
	void markUpdated(const string &key)
	{
		OptionMap::iterator it; 
		if ((it = optionMap.find(key)) != optionMap.end())
		{
			(it->second)++;
		}
	}
#endif
};

class IniReader {
#if 0 

	typedef std::map<string, string> IniValueMap; 
	public:
		IniReader(bool warnOnUnused_=false, bool errorOnUnset_=true)
			: warnOnUnused(warnOnUnused_), errorOnUnset(errorOnUnset_)
		{

		}

		/** 
		 * TODO: update interface -- readFile should return a map of options that
		 * should then be passed to the individual read() functions by finish(),
		 * which should be renamed; Finally, finish() should delete the map that
		 * it was passed.  This would allow for more flexible overrides from a
		 * different source other than the ini file
		 */
		
		/**
		 * Read a file and put the key-value pairs into the 'allOptions' map
		 */
		bool readFile(const string &filename, PHXSim::Config &cfg)
		{
			ifstream iniFile;
			size_t lineNumber=0;
			string line; 
			iniFile.open(filename.c_str());
			if (iniFile.fail())
			{
				ERROR("Can't open '"<<filename<<"'"); 
				abort();
			}
			cerr << "Reading ini file '"<<filename<<"'\n";
			if (iniFile.is_open())
			{
	
				while (!iniFile.eof())
				{

					lineNumber++;
					getline(iniFile, line);

					// get rid of everything after a ;
					list<string> splitTemp = split(line, ";"); 
					if (!splitTemp.empty())
					{
						line = splitTemp.front();
						//				cerr << "truncating line after comment to '"<<line<<"'\n";
					}

					// if the string is now empty, don't bother parsing it
					if (line.empty())
					{
						//				std::cerr << "Skipping blank line "<<lineNumber<<std::endl;
						continue;
					}
					splitTemp = split(line, "="); 
					if (splitTemp.size() != 2)
					{
						//				std::cerr << "Malformed line: '"<<line<<"' ("<<lineNumber<<")"<<splitTemp.size()<<"\n";
						continue; 
					}
					else
					{
						const char *strip_chars=" \t";
						string key = strip(splitTemp.front(), strip_chars); 
						string value = strip(splitTemp.back(), strip_chars); 
						if (!cfg.hasOption(key)) {
							std::cerr<<"WARNING: line "<<lineNumber<<" has invalid key "<<key<<std::endl;
						}
						allOptions[key] = value;
					}
				}
			}
			else
			{
				return false;
			}
			return true;
		}
		/** 
		 * Individual 'read' functions which get the key out of the 'allOptions'
		 * map and write them to the pointer provided. Overloaded based on the 
		 * destination pointer type
		 */
		template <class T>
		bool read(const string &key, T &value)
		{
			string *valueStr = getValue(key); 
			if (!valueStr) {
				return false; 
			}

			istringstream iss(*valueStr); 
			T tmp; 
			iss >> tmp;
			if (!iss) {
				return false; 
			}
			value = tmp;
			return true;
		}

		void markAsUpdated(PHXSim::Config &cfg, const IniValueMap &keyPairs) 
		{
			IniValueMap::const_iterator it=keyPairs.begin();
			for ( ; it != keyPairs.end(); it++)
			{
				cfg.markUpdated(it->first); 
			}
		}


		bool read(const string &key, bool &value)
		{
			string *valueStr = getValue(key); 
			if (!valueStr) {
				return false; 
			}
			value=true;
			if (valueStr->compare("True") == 0 || valueStr->compare("true") == 0)
				value = true; 
			else
				value = false;

			return true; 
		}

		bool read(const string &key, char *&value)
		{
			string *valueStr = getValue(key); 
			if (!valueStr) {
				return false; 
			}
			value = strndup(valueStr->c_str(), valueStr->length()+1); 
			return true; 
		}

		bool finish(PHXSim::Config &cfg)
		{
			bool success = true; 
			#define PARAM(type, var_name, default_var) success &= read(#var_name, cfg.var_name);
			#include "config.def"
			markAsUpdated(cfg, allOptions);
			return success;
		}

	private:
		map<string, string> allOptions;
		bool warnOnUnused;
		bool errorOnUnset;

		/** 
		 * Individual 'read' functions which get the key out of the 'allOptions'
		 * map and write them to the pointer provided. Overloaded based on the 
		 * destination pointer type
		 */
		string *getValue(const string &key)
		{
			map<string,string>::iterator it; 
			if ((it = allOptions.find(key)) != allOptions.end())
			{
				return &(it->second);
			}
			else
			{
				if (errorOnUnset)
				{
					std::cerr << "ERROR: key '"<<key<<"' is unset\n";
					abort(); 
				}
				else
				{
					return NULL;
				}
			}
		}
#endif
};

#endif // _INIREADER_H_
