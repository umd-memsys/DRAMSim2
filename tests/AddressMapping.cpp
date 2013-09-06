#include <gtest/gtest.h>
#include <stdint.h>
#include <NewAddressMapping.h>

using namespace DRAMSim;
TEST(AddressMapper, basic) {
	unsigned fieldOrder[END_FIELD] = {COL_LO_FIELD, CHANNEL_FIELD, RANK_FIELD, BANK_FIELD, ROW_FIELD, COL_HI_FIELD};
	AddressMapper mapper(fieldOrder);
	uint64_t input=3;
	Address output(input); 
	ASSERT_EQ(output.address, input);
	ASSERT_EQ(output.colLo, 0U);

	mapper.map(output); 
	ASSERT_EQ(output.colLo, 3U);
	ASSERT_EQ(output.channel, 0U);
	ASSERT_EQ(output.rank, 0U);
	ASSERT_EQ(output.bank, 0U);
	ASSERT_EQ(output.row, 0U);
	ASSERT_EQ(output.colHi, 0U);

	unsigned fieldOrder2[END_FIELD] = {CHANNEL_FIELD, COL_LO_FIELD, RANK_FIELD, BANK_FIELD, ROW_FIELD, COL_HI_FIELD};
	AddressMapper mapper2(fieldOrder2);
	mapper2.map(output);
	ASSERT_EQ(output.channel, 1U);
	ASSERT_EQ(output.colLo, 1U);
	ASSERT_EQ(output.rank, 0U);
	ASSERT_EQ(output.bank, 0U);
	ASSERT_EQ(output.row, 0U);
	ASSERT_EQ(output.colHi, 0U);

}
