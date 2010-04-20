//SimulatorObject.cpp
//
//Base class for all classes in the simulator
//	

#include <cstdlib>
#include "SimulatorObject.h"

using namespace DRAMSim;
using namespace std;

void SimulatorObject::step()
{
	currentClockCycle++;
}


