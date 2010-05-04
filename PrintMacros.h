#ifndef PRINT_MACROS_H
#define PRINT_MACROS_H

extern int SHOW_SIM_OUTPUT; //enable or disable PRINT() statements -- set by flag in TraceBasedSim.cpp

#define ERROR(str) std::cerr<<"[ERROR ("<<__FILE__<<":"<<__LINE__<<")]: "<<str<<std::endl;


#ifdef DEBUG_BUILD
#define DEBUG(str)  std::cout<< str <<std::endl;
#define DEBUGN(str) std::cout<< str;
#else
#define DEBUG(str) ;
#define DEBUGN(str) ;
#endif

#ifndef NO_OUTPUT
#define PRINT(str)  if(SHOW_SIM_OUTPUT) { std::cout <<str<<std::endl; }
#define PRINTN(str) if(SHOW_SIM_OUTPUT) { std::cout <<str; }
#else
#undef DEBUG
#undef DEBUGN
#define DEBUG(str) ;
#define DEBUGN(str) ;
#define PRINT(str) ;
#define PRINTN(str) ;
#endif

#endif
