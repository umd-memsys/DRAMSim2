//BusPacket.cpp
//
//Class file for bus packet object
//

#include "BusPacket.h"

using namespace DRAMSim;
using namespace std;

BusPacket::BusPacket(BusPacketType packtype, uint64_t physicalAddr, uint col, uint rw, uint r, uint b, void *dat)
{
	physicalAddress = physicalAddr;
	busPacketType = packtype;
	data = dat;
	rank = r;
	bank = b;
	column = col;
	row = rw;
}

BusPacket::BusPacket() {}
void BusPacket::print(uint64_t currentClockCycle, bool dataStart) 
{
	if (this == NULL) {
		return;
	}

	if (VERIFICATION_OUTPUT) 
	{
		switch(busPacketType)
		{
			case READ:
				cmd_verify_out << currentClockCycle << ": read ("<<rank<<","<<bank<<","<<column<<",0);"<<endl;
				break;
			case READ_P:
				cmd_verify_out << currentClockCycle << ": read ("<<rank<<","<<bank<<","<<column<<",1);"<<endl;
				break;
			case WRITE:
				cmd_verify_out << currentClockCycle << ": write ("<<rank<<","<<bank<<","<<column<<",0 , 0, 'h0);"<<endl; 
				break;
			case WRITE_P:
				cmd_verify_out << currentClockCycle << ": write ("<<rank<<","<<bank<<","<<column<<",1, 0, 'h0);"<<endl;
				break;
			case ACTIVATE:
				cmd_verify_out << currentClockCycle <<": activate (" << rank << "," << bank << "," << row <<");"<<endl;
				break;
			case PRECHARGE:
				cmd_verify_out << currentClockCycle <<": precharge (" << rank << "," << bank << "," << row <<");"<<endl;
				break;
			case REFRESH:
				cmd_verify_out << currentClockCycle <<": refresh (" << rank << ");"<<endl; 
				break;
			case DATA:
				//TODO: data verification?
				break;
			default:
				ERROR("Trying to print unknown kind of bus packet");
				exit(-1);
		}
	} 
}
void BusPacket::print()
{
	if (this == NULL) //pointer use makes this a necessary precaution
	{
		return;
	}
else {
		switch(busPacketType)
		{
			case READ:
				PRINT("BP [READ] pa[0x"<<hex<<physicalAddress<<dec<<"] r["<<rank<<"] b["<<bank<<"] row["<<row<<"] col["<<column<<"]");
				break;
			case READ_P:
				PRINT("BP [READ_P] pa[0x"<<hex<<physicalAddress<<dec<<"] r["<<rank<<"] b["<<bank<<"] row["<<row<<"] col["<<column<<"]");
				break;
			case WRITE:
				PRINT("BP [WRITE] pa[0x"<<hex<<physicalAddress<<dec<<"] r["<<rank<<"] b["<<bank<<"] row["<<row<<"] col["<<column<<"]");
				break;
			case WRITE_P:
				PRINT("BP [WRITE_P] pa[0x"<<hex<<physicalAddress<<dec<<"] r["<<rank<<"] b["<<bank<<"] row["<<row<<"] col["<<column<<"]");
				break;
			case ACTIVATE:
				PRINT("BP [ACT] pa[0x"<<hex<<physicalAddress<<dec<<"] r["<<rank<<"] b["<<bank<<"] row["<<row<<"] col["<<column<<"]");
				break;
			case PRECHARGE:
				PRINT("BP [PRE] pa[0x"<<hex<<physicalAddress<<dec<<"] r["<<rank<<"] b["<<bank<<"] row["<<row<<"] col["<<column<<"]");
				break;
			case REFRESH:
				PRINT("BP [REF] pa[0x"<<hex<<physicalAddress<<dec<<"] r["<<rank<<"] b["<<bank<<"] row["<<row<<"] col["<<column<<"]");
				break;
			case DATA:
				PRINTN("BP [DATA] pa[0x"<<hex<<physicalAddress<<dec<<"] r["<<rank<<"] b["<<bank<<"] row["<<row<<"] col["<<column<<"] data["<<data<<"]=");
				BusPacket::printData(data);
				PRINT("");
				break;
			default:
				ERROR("Trying to print unknown kind of bus packet");
				exit(-1);
		}
	}
}

void BusPacket::printData(const void *data) 
{
	if (data == NULL) 
	{
		PRINTN("NO DATA");
		return;
	}
	PRINTN("'" << hex);
	for (int i=0; i < 4; i++) 
	{
		PRINTN(((uint64_t *)data)[i]);
	}
	PRINTN("'" << dec);
}
