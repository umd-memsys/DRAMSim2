#ifndef TRANSACTION_H
#define TRANSACTION_H

//Transaction.h
//
//Header file for transaction object

#include "SystemConfiguration.h"

using namespace std;

namespace DRAMSim
{
	enum TransactionType
	{
		DATA_READ, 
		DATA_WRITE,
		RETURN_DATA
	};

	class Transaction
	{
	public:
		//fields
		TransactionType transactionType;
		uint64_t address;
		void *data;
		uint64_t timeAdded;
		uint64_t timeReturned;

		
		//functions
		Transaction(TransactionType transType, uint64_t addr, void *data);
		Transaction();
		
		void print();
	};
}

#endif

