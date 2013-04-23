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

	unsigned refPd = cfg.REFRESH_PERIOD; 
	ASSERT_EQ(refPd, 100U);
	cfg.REFRESH_PERIOD.set("49");
	refPd = cfg.REFRESH_PERIOD; 
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

TEST(ConfigOptions, set_by_map) {
	
	DRAMSim::Config cfg; 
	DRAMSim::Config::OptionsMap map; 
	DRAMSim::Config::OptionsSuccessfullySetMap successes; 

	map["tCK"] = "2.5"; 

	float tCK = cfg.tCK; 
	ASSERT_EQ(tCK, 1.5f);
	successes = cfg.set(map);
	ASSERT_EQ(successes.size(), 1U);
	tCK = cfg.tCK; 
	ASSERT_EQ(tCK, 2.5f);

	map["tCK"] = "5.0"; 
	map["REFRESH_PERIOD"] = "500"; 
	map["bogus"] = "234";

	successes = cfg.set(map);
	ASSERT_EQ(successes.size(), 2U);

	tCK = cfg.tCK; 
	ASSERT_EQ(tCK, 5.0f);
	ASSERT_EQ((unsigned)cfg.REFRESH_PERIOD, 500U); 
}
