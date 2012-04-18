#include "../MultiChannelMemorySystem.h"
#include "../CommandQueue.h"
#include "../MemorySystem.h"
#include "../MemoryController.h"

#include <gtest/gtest.h>

namespace {

	using namespace DRAMSim;
	class CQTester
	{
		public: 
			MultiChannelMemorySystem *m;
			CQTester() 
			{
				m = new MultiChannelMemorySystem("ini/DDR3_micron_64M_8B_x4_sg15.ini", "system.ini.example", "/home/prosenf1/DRAMSim2", "gtest", 4096, new string("gtest"));
			}


	};

	TEST(CommandQueue, CapacityTest)
	{
		CQTester t;
		CommandQueue &cq = t.m->channels[0]->memoryController->commandQueue;
		t.m->addTransaction(false, 0); 
		ASSERT_EQ(cq.getCommandQueue(0,0).size(),0); 
		t.m->update(); 
		ASSERT_EQ(cq.getCommandQueue(0,0).size(),2); 
		t.m->update(); 

	}


}
