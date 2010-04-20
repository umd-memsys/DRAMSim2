#ifndef SIMULATOROBJ_H
#define SIMULATOROBJ_H

//SimulatorObject.h
//
//Header file for simulator object class
//

#include <stdint.h>

namespace DRAMSim
{
	class SimulatorObject
	{
	public:
		uint64_t currentClockCycle;
		
		void step();
		virtual void update()=0;
	};
}

#endif

