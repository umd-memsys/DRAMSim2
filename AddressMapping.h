#ifndef ADDRESS_MAPPING_H
#define ADDRESS_MAPPING_H
namespace DRAMSim
{
	void addressMapping(uint64_t physicalAddress, unsigned &channel, unsigned &rank, unsigned &bank, unsigned &row, unsigned &col);
}

#endif
