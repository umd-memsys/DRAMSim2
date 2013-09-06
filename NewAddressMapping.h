#include <string.h> //memcpy

namespace DRAMSim {
	enum AddressFields {
		CHANNEL_FIELD=0,
		RANK_FIELD, 
		BANK_FIELD, 
		ROW_FIELD, 
		COL_HI_FIELD, 
		COL_LO_FIELD, 
		END_FIELD
	};

	struct Address {
		Address(uint64_t address_) 
			: address(address_), 
			channel(fields[CHANNEL_FIELD]), 
			rank(fields[RANK_FIELD]), 
			bank(fields[BANK_FIELD]), 
			row(fields[ROW_FIELD]), 
			colHi(fields[COL_HI_FIELD]), 
			colLo(fields[COL_LO_FIELD])
		{
			memset(fields, 0, sizeof(fields));
		}
		uint64_t address;
		unsigned fields[END_FIELD];
		unsigned &channel;
		unsigned &rank; 
		unsigned &bank;
		unsigned &row;
		unsigned &colHi; 
		unsigned &colLo;
	};


	class AddressMapper {

		static inline uint64_t MakeMask(unsigned startBit, unsigned fieldWidth) {
			uint64_t mask = (1UL << fieldWidth)-1; 
			return (0UL | (mask << startBit)); 
		}

		static inline unsigned ExtractField(uint64_t addr, unsigned startBit, unsigned fieldWidth) {
			return (unsigned)((addr & MakeMask(startBit, fieldWidth)) >> startBit); 
		}
		static inline uint64_t PutField(uint64_t addr, unsigned fieldValue, unsigned startBit, unsigned fieldWidth) {
			fieldValue &= MakeMask(0, fieldWidth);
			addr |= (fieldValue<<startBit);
			return addr;
		}

		unsigned fieldOrder[END_FIELD]; 
		unsigned fieldWidths[END_FIELD];
		
		public: 
		AddressMapper(unsigned *fieldOrder_) {
			// save the field order
			memcpy(fieldOrder, fieldOrder_, sizeof(fieldOrder));

			// set the field widths
			fieldWidths[CHANNEL_FIELD] = 1; 
			fieldWidths[RANK_FIELD] = 2;
			fieldWidths[BANK_FIELD] = 3; 
			fieldWidths[ROW_FIELD] = 14;
			fieldWidths[COL_HI_FIELD] = 10; 
			fieldWidths[COL_LO_FIELD] = 2;
		}

		Address &create(uint64_t address) const {
			Address *output = new Address(address);
			map(*output); 
			return *output; 
		}

		void map(Address &address) const {
			uint64_t inputAddress = address.address;
			unsigned bitCounter=0;
			for (unsigned i=0; i<END_FIELD; ++i) {
				unsigned fieldIndex = fieldOrder[i];
				unsigned fieldWidth = fieldWidths[fieldIndex];
				unsigned field = ExtractField(inputAddress, bitCounter, fieldWidth);
				address.fields[fieldIndex] = field;
				bitCounter += fieldWidth;
			}
		}

		uint64_t getAddress(const Address &address) const {
			uint64_t outputAddress=0;
			unsigned bitCounter=0;
			for (unsigned i=0; i<END_FIELD; ++i) {
				unsigned fieldIndex = fieldOrder[i];
				unsigned fieldWidth = fieldWidths[fieldIndex];
				unsigned field = address.fields[fieldIndex];
				outputAddress = PutField(outputAddress, field, bitCounter, fieldWidth); 
				bitCounter+=fieldWidth;
			}
			return outputAddress;
		}
	};
}
