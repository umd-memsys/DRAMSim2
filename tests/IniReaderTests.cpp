#include <gtest/gtest.h>
#include <ConfigIniReader.h>
#include <string>

using std::string; 
TEST(ConfigOptions, setters_and_getters) {
	DRAMSim::Config cfg; 

	cfg.tCK.set(string("1.7"));
	float tCK = cfg.tCK; 
	ASSERT_EQ(tCK, 1.7f);

	cfg.tCK.set(2.0f);
	float tCK2 = cfg.tCK; 
	ASSERT_EQ(tCK2, 2.0f);

	cfg.REFRESH_PERIOD.set("49");
	unsigned refPd = cfg.REFRESH_PERIOD; 
	ASSERT_EQ(refPd, 49U);

	// An enum example
	DRAMSim::RowBufferPolicy rbp = cfg.rowBufferPolicy;
	ASSERT_EQ(rbp, DRAMSim::OpenPage);
	cfg.rowBufferPolicy.set("ClosePage");
	rbp = cfg.rowBufferPolicy;

	if (cfg.tCK != 2.0f) {
		ASSERT_TRUE(false);
	}

}

TEST(ConfigOptions, setters) {
	
	DRAMSim::Config cfg; 

	float tCK = cfg.tCK; 
	ASSERT_EQ(tCK, 1.5f);
	bool success = cfg.set("tCK", "2.5");
	ASSERT_TRUE(success);
	tCK = cfg.tCK; 
	ASSERT_EQ(tCK, 2.5f);

}
