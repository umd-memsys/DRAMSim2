#include <gtest/gtest.h>
#include <DRAMSim.h>
#include <CSVWriter.h>
#include <StatsDRAMSimInterface.h>
#include <string>

using std::string; 
using namespace DRAMSim;

struct TransactionReceiver {
	void readDone(unsigned id, uint64_t addr, uint64_t cycle) {
		std::cout << "Got back read  "<<std::hex<<addr<<" on cycle "<<cycle<<"\n"; 
	}
	void writeDone(unsigned id, uint64_t addr, uint64_t cycle) {
		std::cout << "Got back write "<<std::hex<<addr<<" on cycle "<<cycle<<"\n"; 
	}
	typedef Callback<TransactionReceiver, void, unsigned, uint64_t, uint64_t> TransactionCompleteCB; 

};
TEST(StatsIniReader, instantiation) {
	CSVWriter &CSVOut = CSVWriter::GetCSVWriterInstance("test.csv");
	DRAMSimInterface *mem = getMemorySystemInstance("ini/DDR3_micron_32M_8B_x4_sg125.ini", "system.ini","/home/prosenf1/DRAMSim2", "test", 4096, CSVOut);
	StatsDRAMSimInterface statsMem(mem);
#if 0 
	TransactionReceiver tr; 

	TransactionCompleteCB *readDone = new TransactionReceiver::TransactionCompleteCB(&tr, &TransactionReceiver::readDone);
	TransactionCompleteCB *writeDone = new TransactionReceiver::TransactionCompleteCB(&tr, &TransactionReceiver::writeDone);

	statsMem.registerCallbacks(readDone, writeDone, NULL); 

#endif
	statsMem.dumpStats(CSVOut);
	CSVOut.finalize();
	statsMem.addTransaction(false, 0x3); 
	for (unsigned i=0; i<100; ++i) {
		statsMem.update(); 
	}
	statsMem.dumpStats(CSVOut);
	CSVOut.finalize();

	statsMem.addTransaction(true, 0x5); 
	for (unsigned i=0; i<100; ++i) {
		statsMem.update(); 
	}
	statsMem.dumpStats(CSVOut);
	CSVOut.finalize();
}

