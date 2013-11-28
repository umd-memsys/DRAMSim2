/*********************************************************************************
*  Copyright (c) 2010-2011, Paul Rosenfeld
*                             Elliott Cooper-Balis
*                             Bruce Jacob
*                             University of Maryland 
*                             dramninjas [at] gmail [dot] com
*  All rights reserved.
*  
*  Redistribution and use in source and binary forms, with or without
*  modification, are permitted provided that the following conditions are met:
*  
*     * Redistributions of source code must retain the above copyright notice,
*        this list of conditions and the following disclaimer.
*  
*     * Redistributions in binary form must reproduce the above copyright notice,
*        this list of conditions and the following disclaimer in the documentation
*        and/or other materials provided with the distribution.
*  
*  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
*  ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
*  WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
*  DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
*  FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
*  DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
*  SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
*  CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
*  OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
*  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*********************************************************************************/

#ifndef _ADDRESS_MAPPING_H_
#define _ADDRESS_MAPPING_H_

#include <iostream>
#include <assert.h>
using std::ostream;


namespace DRAMSim {

class Config;

enum AddressFields {
	CHANNEL_FIELD=0,
	RANK_FIELD, 
	BANK_FIELD, 
	ROW_FIELD, 
	COL_HI_FIELD,
	COL_LO_FIELD, 
	OFFSET_FIELD, 
	END_FIELD
};

struct Address {
	const uint64_t physicalAddress; 

	unsigned fields[END_FIELD];

	unsigned &channel; 
	unsigned &rank;
	unsigned &bank;
	unsigned &row;
	unsigned &colHi;
	unsigned &colLo;

	// These will be computed after a mapping based on the other fields
	unsigned col;


	explicit Address(uint64_t addr_);

	/**
	 * Allow casting to a uint64_t to just return the address
	 */
	operator uint64_t() const {
		return physicalAddress;
	}

	bool operator==(const Address &other) const;
	ostream &print(ostream &out) const;
	friend class AddressMapper; 
};

ostream &operator<<(ostream &out, const Address &addr);

class AddressMapper {
	const Config &cfg;
	unsigned fieldWidths[END_FIELD]; 
	unsigned fieldOrder[END_FIELD];

	public:
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
	AddressMapper (const Config &cfg_, unsigned *fieldOrder_);


	/**
	 * Forward mapping: read an Address's physicalAddress and set its fields correctly
	 */
	void map(Address &addr, unsigned transactionSize);

	/** 
	 * Read a string of fields like "r:b:rw:by" and generate a field order array. 
	 * This in turn can be used to construct an AddressMapper 
	 */
	static unsigned *FieldOrderFromString(const std::string &str, bool verbose=false);
};


} // namespace 
#endif
