/*********************************************************************************
*  Copyright (c) 2010-2013,  Paul Rosenfeld
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

#include <assert.h>
#include "SystemConfiguration.h"
#include "Util.h"
#include "util.h"
#include "AddressMapping.h"
#include "ConfigIniReader.h"
#include <string.h> //memset

using namespace std; 

namespace DRAMSim {

	const char * FieldStrings[] = {
		"chan",
		"rk",
		"bk",
		"rw",
		"ch",
		"cl",
		"of",
		NULL
	};
	/************************ Address *********************/


	Address::Address(uint64_t addr_)
		: physicalAddress(addr_)
		, channel(fields[CHANNEL_FIELD])
		, rank(fields[RANK_FIELD])
		, bank(fields[BANK_FIELD])
		, row(fields[ROW_FIELD])
		, colHi(fields[COL_HI_FIELD])
		, colLo(fields[COL_LO_FIELD])
	{
		memset(fields, 0, sizeof(fields));
	}

	bool Address::operator==(const Address &other) const {
		return physicalAddress == other.physicalAddress;
	}

	ostream &Address::print(ostream &out) const {
		out << "0x"<<std::hex << physicalAddress << std::dec<< " [c: "<<channel<<", rk: "<<rank<<", b: "<<bank<<", r:"<<row<<", c:"<<col<<"]";
		return out; 
	}

	ostream &operator<<(ostream &out, const Address &addr) {
		return addr.print(out);
	}
/********************* AddressMapper ********************/
	AddressMapper::AddressMapper (const Config &cfg_, unsigned *fieldOrder_) 
		: cfg(cfg_) 
	{
		memcpy(fieldOrder, fieldOrder_, sizeof(fieldOrder));

		fieldWidths[CHANNEL_FIELD]  = dramsim_log2(cfg.NUM_CHANS);
		fieldWidths[RANK_FIELD]  = dramsim_log2(cfg.NUM_RANKS);
		fieldWidths[BANK_FIELD]  = dramsim_log2(cfg.NUM_BANKS);
		fieldWidths[ROW_FIELD]   = dramsim_log2(cfg.NUM_ROWS);

		if (cfg.DEBUG_ADDR_MAP) 
			PRINT("Widths: channel:"<<fieldWidths[CHANNEL_FIELD]<<" rank:"<<fieldWidths[RANK_FIELD]<<" bank:"<<fieldWidths[BANK_FIELD]<<" row:"<<fieldWidths[ROW_FIELD]<<" colHi:"<<fieldWidths[COL_HI_FIELD]<<" colLow:"<<fieldWidths[COL_LO_FIELD]<<" offset:"<<fieldWidths[OFFSET_FIELD]<<endl);
	}

	void AddressMapper::map(Address &address, unsigned transactionSize) {
		// TODO: maybe: make this function const by copying fieldWidths each time
		unsigned burstWidth = dramsim_log2(cfg.JEDEC_DATA_BUS_BITS/8);
		unsigned transactionWidth = dramsim_log2(transactionSize); 
		unsigned colLowWidth = transactionWidth - burstWidth; 
		fieldWidths[COL_LO_FIELD] = colLowWidth; 
		fieldWidths[COL_HI_FIELD] = dramsim_log2(cfg.NUM_COLS) - colLowWidth;
		fieldWidths[OFFSET_FIELD] = transactionWidth - colLowWidth;

		uint64_t inputAddress = address.physicalAddress;

		unsigned bitCounter=0;
		for (unsigned i=0; i<END_FIELD; ++i) {
			unsigned fieldIndex = fieldOrder[i];
			unsigned fieldWidth = fieldWidths[fieldIndex];
			unsigned field = ExtractField(inputAddress, bitCounter, fieldWidth);
			address.fields[fieldIndex] = field;
			bitCounter += fieldWidth;
		}

		address.col = (address.colHi << fieldWidths[COL_LO_FIELD]) + address.colLo;
		
		if (cfg.DEBUG_ADDR_MAP) {
			PRINT("Widths: channel:"<<fieldWidths[CHANNEL_FIELD]<<" rank:"<<fieldWidths[RANK_FIELD]<<" bank:"<<fieldWidths[BANK_FIELD]<<" row:"<<fieldWidths[ROW_FIELD]<<" colHi:"<<fieldWidths[COL_HI_FIELD]<<" colLow:"<<fieldWidths[COL_LO_FIELD]<<" offset:"<<fieldWidths[OFFSET_FIELD]<<endl);
			PRINT("Mapped 0x"<<std::hex<<address.physicalAddress<<std::dec<<" to channel="<<address.channel<<", rank="<<address.rank<<", bank="<<address.bank<<", row="<<address.row<<", col="<<address.col<<" colHi="<<address.colHi<<endl);
		}
	}

	unsigned *AddressMapper::FieldOrderFromString(const std::string &str, bool verbose) {
		list<string> pieces = split(str, ":");
		if (pieces.size() != END_FIELD) {
			ERROR("Your address mapping scheme '"<<str<<"' does not have the proper number of fields; it should have "<<END_FIELD<<" fields but only has "<<pieces.size());
			abort();
		}
		
		unsigned *fieldOrder = new unsigned[END_FIELD];
		unsigned fieldIndex = 0;
		for (list<string>::const_iterator it = pieces.begin(); it != pieces.end(); ++it) {
			const string &piece = *it; 
			if (verbose) {
				PRINT("piece='"<<piece<<"'");
			}
			// TODO: move this inner loop to its own function 
			bool foundMatch=false;
			for (unsigned i=0; i<END_FIELD; ++i) {
				/* if this field matches, set the fieldOrder variable to the equivalent enum value */
				if (piece == FieldStrings[i]) {
					PRINT(piece);
					// i is the enum index
					fieldOrder[fieldIndex] = i;
					if (verbose) {
						PRINT("\t'"<<piece<<"': setting fieldOrder["<<fieldIndex<<"] = " << i);
					}
					foundMatch = true;
					fieldIndex++;
					break;
				}
			}
			assert(foundMatch);
		}
		// make sure that the number of fields matches the number in the string
		assert(fieldIndex == END_FIELD);
		return(fieldOrder);
	}

} // namespace
