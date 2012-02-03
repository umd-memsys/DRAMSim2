#ifndef _DATA_PACKET_H_
#define _DATA_PACKET_H_

#include <string.h> //memcpy 
#include <stdint.h>
#include <stdlib.h> //free
#include <iostream>

using std::ostream; 
using std::dec;
using std::hex;

namespace DRAMSim {
	typedef unsigned char byte;

	class DataPacket {
	/*
	 * A very thin wrapper around a data that is sent and received in DRAMSim2
	 *
	 */
	private:
		// Disable copying for a datapacket
		DataPacket(const DataPacket &other);
		DataPacket &operator =(const DataPacket &other); 

	public: 
		/**
		 * Constructor to be used if we are using NO_STORAGE
		 *
		 */
		DataPacket() : _data(NULL), _numBytes(0), _unalignedAddr(0)
		{}

		/**
		 * @param data pointer to a buffer of data; DRAMSim will take ownership of the buffer and will be responsible for freeing it 
		 * @param numBytes number of bytes in the data buffer
		 * @param unalignedAddr DRAMSim will typically kill the bottom bits to align them to the DRAM bus width, but if an access is unaligned (and smaller than the transaction size, the raw address will need to be known to properly execute the read/write)
		 *
		 */
		DataPacket(byte *data, size_t numBytes, uint64_t unalignedAddr) :
			_data(data), _numBytes(numBytes), _unalignedAddr(unalignedAddr)
		{}
		virtual ~DataPacket()
		{
			if (_data)
			{
				free(_data); 
			}
		}

		// accessors
		size_t getNumBytes() const
		{
			return _numBytes; 
		}
		byte *getData() const
		{
			return _data;
		}
		uint64_t getAddr() const
		{
			return _unalignedAddr; 
		}
		void setData(const byte *data, size_t size)
		{
			_data = (byte *)calloc(size,sizeof(byte));
			memcpy(_data, data, size);
			_numBytes = size; 
		}
		bool hasNoData() const
		{
			return (_data == NULL || _numBytes == 0);
		}

		friend ostream &operator<<(ostream &os, const DataPacket &dp);

	private:
		byte *_data; 
		size_t _numBytes;
		uint64_t _unalignedAddr;
	};


} // namespace DRAMSim

#endif // _DATA_PACKET_H_
