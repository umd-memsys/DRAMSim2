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
	ASSERT_EQ(refPd, 7800U);
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
	DRAMSim::OptionsMap map; 
	DRAMSim::OptionsFailedToSet failures; 


	float tCK = cfg.tCK; 
	ASSERT_EQ(tCK, 1.25f);

	map["tCK"] = "2.5"; 
	failures = cfg.set(map);
	ASSERT_EQ(failures.size(), 0U);
	tCK = cfg.tCK; 
	ASSERT_EQ(tCK, 2.5f);

	map["tCK"] = "5.0"; 
	map["REFRESH_PERIOD"] = "500"; 
	map["bogus"] = "234";
	map["DEBUG_ADDR_MAP"] = "true";

	ASSERT_FALSE((bool) cfg.DEBUG_ADDR_MAP); 
	failures = cfg.set(map);
	ASSERT_EQ(failures.size(), 1U);
	ASSERT_STREQ(failures.begin()->c_str(), "bogus");
	ASSERT_TRUE((bool) cfg.DEBUG_ADDR_MAP); 

	tCK = cfg.tCK; 
	ASSERT_EQ(tCK, 5.0f);
	ASSERT_EQ((unsigned)cfg.REFRESH_PERIOD, 500U); 
}
