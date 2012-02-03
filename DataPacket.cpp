#include "DataPacket.h"
#include <stdio.h>

namespace DRAMSim 
{

	ostream &operator<<(ostream &os, const DataPacket &dp)
	{
		if (dp.hasNoData())
		{
			os << "NO DATA";
			return os;  
		}
		char tmp[8];
		os << "DATA: ("<< dp._numBytes << ") '";
		for (size_t i=0; i < dp._numBytes; i++)
		{
			// this is easier than trying to figure out how to do it with a stream
			snprintf(tmp, 8, "%02x ", dp._data[i]); 
			os << tmp;
		}
		os << "'";

		return os;
	}

}
