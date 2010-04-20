#ifndef INIREADER_H
#define INIREADER_H

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include "SystemConfiguration.h"

using namespace std;

// Uhhh, apparently the #name equals "name" -- HOORAY MACROS!
#define DEFINE_UINT_PARAM(name, paramtype) {#name, &name, UINT, paramtype, false}
#define DEFINE_STRING_PARAM(name, paramtype) {#name, &name, STRING, paramtype, false}
#define DEFINE_FLOAT_PARAM(name,paramtype) {#name, &name, FLOAT, paramtype, false}
#define DEFINE_BOOL_PARAM(name, paramtype) {#name, &name, BOOL, paramtype, false}
#define DEFINE_UINT64_PARAM(name, paramtype) {#name, &name, UINT64, paramtype, false}

namespace DRAMSim 
{

	typedef enum _variableType {STRING, UINT, UINT64, FLOAT, BOOL} varType;
	typedef enum _paramType {SYS_PARAM, DEV_PARAM} paramType;
	typedef struct _configMap 
	{
		string iniKey; //for example "tRCD"

		void *variablePtr;
		varType variableType;
		paramType parameterType;
		bool wasSet; 
	} ConfigMap;

	class IniReader 
	{		
		public:
			static void SetKey(string key, string value, bool isSystemParam = false, size_t lineNumber = 0);
			static void OverrideKeys(vector<string> keys, vector<string> values);
			static void ReadIniFile(string filename, bool isSystemParam);
			static void InitEnumsFromStrings();
			static bool CheckIfAllSet();
			static void WriteValuesOut(std::ofstream &visDataOut);
		private:
			static void Trim(string &str);
	};
}


#endif
