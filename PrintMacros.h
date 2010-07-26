/****************************************************************************
*	 DRAMSim2: A Cycle Accurate DRAM simulator 
*	 
*	 Copyright (C) 2010   	Elliott Cooper-Balis
*									Paul Rosenfeld 
*									Bruce Jacob
*									University of Maryland
*
*	 This program is free software: you can redistribute it and/or modify
*	 it under the terms of the GNU General Public License as published by
*	 the Free Software Foundation, either version 3 of the License, or
*	 (at your option) any later version.
*
*	 This program is distributed in the hope that it will be useful,
*	 but WITHOUT ANY WARRANTY; without even the implied warranty of
*	 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*	 GNU General Public License for more details.
*
*	 You should have received a copy of the GNU General Public License
*	 along with this program.  If not, see <http://www.gnu.org/licenses/>.
*
*****************************************************************************/



#ifndef PRINT_MACROS_H
#define PRINT_MACROS_H

extern int SHOW_SIM_OUTPUT; //enable or disable PRINT() statements -- set by flag in TraceBasedSim.cpp

#define ERROR(str) std::cerr<<"[ERROR ("<<__FILE__<<":"<<__LINE__<<")]: "<<str<<std::endl;


#ifdef DEBUG_BUILD
	#define DEBUG(str)  std::cerr<< str <<std::endl;
	#define DEBUGN(str) std::cerr<< str;
#else
	#define DEBUG(str) ;
	#define DEBUGN(str) ;
#endif

#ifdef NO_OUTPUT
	#undef DEBUG
	#undef DEBUGN
	#define DEBUG(str) ;
	#define DEBUGN(str) ;
	#define PRINT(str) ;
	#define PRINTN(str) ;
#else
	#ifdef LOG_OUTPUT
		namespace DRAMSim {
		extern std::ofstream dramsim_log;
		}
		#define PRINT(str)  { DRAMSim::dramsim_log <<str<<std::endl; }
		#define PRINTN(str) { DRAMSim::dramsim_log <<str; }
	#else
		#define PRINT(str)  if(SHOW_SIM_OUTPUT) { std::cout <<str<<std::endl; }
		#define PRINTN(str) if(SHOW_SIM_OUTPUT) { std::cout <<str; }
	#endif
#endif

#endif /*PRINT_MACROS_H*/
