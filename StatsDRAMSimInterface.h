#include <list>
#include "DRAMSim.h"
#include "Callback.h"
#include "CSVWriter.h"

using std::list;
namespace DRAMSim {

	/**
	 * A thin wrapper around a standard DRAMSimInterface that collects stats as
	 * requests traverse through the main memory system boundary
	 *
	 */
	class StatsDRAMSimInterface : public DRAMSimInterface {
		DRAMSimInterface *mem; 
		TransactionCompleteCB *readDoneCB; 
		TransactionCompleteCB *writeDoneCB; 
		unsigned requestSize;

		public:
		struct Stats { 
			uint64_t lastDumpCycle;
			unsigned incomingReads;
			unsigned incomingWrites;
			unsigned finishedReads; 
			unsigned finishedWrites; 
			Stats() {
				reset(0);
			}
			void reset(uint64_t lastDumpCycle_) {
				memset(this, 0, sizeof(*this));
				lastDumpCycle=lastDumpCycle_; 
			}
		};

		Stats stats;
		bool dumpOwnStats;
		unsigned dumpIntervalCycles;
		CSVWriter &CSVOut;


		StatsDRAMSimInterface(DRAMSimInterface *dramsimInterface_, CSVWriter &CSVOut_, bool dumpOwnStats_=true) :
			mem(dramsimInterface_), 
			readDoneCB(NULL), 
			writeDoneCB(NULL), 
			requestSize(0U), 
			dumpOwnStats(dumpOwnStats_), 
			dumpIntervalCycles(-1U),
			CSVOut(CSVOut_)
		{
			std::cout << "Wrapping dramsim interface with stats"<<std::endl;
			if (dumpOwnStats) {
				float cyclesPerMs = (1E-3 / mem->getUpdateClockPeriod()); 
				dumpIntervalCycles = static_cast<unsigned>(cyclesPerMs);
				std::cout << " -- own stats with interval " << dumpIntervalCycles<<std::endl;
			}

		}
		uint64_t getCycle() {
			return mem->getCycle();
		}

		bool willAcceptTransaction(bool isWrite, uint64_t addr, unsigned requestSize, unsigned channelId, unsigned coreID) {
			return mem->willAcceptTransaction(isWrite, addr, requestSize, channelId, coreID); 
		}

		bool addTransaction(bool isWrite, uint64_t addr, unsigned requestSize=64, unsigned channelIdx=100, unsigned coreID=0) {
			if (requestSize > 0) {
				this->requestSize = requestSize;
			}
			else if (this->requestSize != requestSize) {
				std::cout<<"ERROR: variable request sizes aren't supported yet"<<std::endl;
				abort();
			}

			bool accepted = mem->addTransaction(isWrite, addr, requestSize, channelIdx, coreID); 
			if (accepted) { 
				if (isWrite) {
					stats.incomingWrites++;
				}
				else {
					stats.incomingReads++;
				}
			}
			return accepted;
		}
		void update() {
			if (dumpOwnStats && mem->getCycle() % dumpIntervalCycles == 0) {
				dumpStats(CSVOut);
				CSVOut.finalize();
			}
			mem->update();
		}

		void setCPUClockSpeed(uint64_t cpuClkFreqHz) {
			mem->setCPUClockSpeed(cpuClkFreqHz);
		}

		void simulationDone() {
            dumpStats(CSVOut);
            CSVOut.finalize();
			mem->simulationDone();
		}

		float getUpdateClockPeriod() {
			return mem->getUpdateClockPeriod();
		}

		void dumpStats(CSVWriter &CSVOut) {
			float elapsedSeconds = (getCycle() - stats.lastDumpCycle)  * getUpdateClockPeriod(); 
			CSVOut << "elapsed_seconds" << elapsedSeconds;
			CSVOut << "ms" << (getCycle() * getUpdateClockPeriod()) * 1000.0f;
			CSVOut << "statsBandwidth" << ((stats.finishedReads + stats.finishedWrites) * requestSize)/elapsedSeconds/1E9;
			mem->dumpStats(CSVOut);
			stats.reset(getCycle());
		}

		void registerCallbacks(
				TransactionCompleteCB *readDone,
				TransactionCompleteCB *writeDone,
				void (*reportPower)(double bgpower, double burstpower, double refreshpower, double actprepower)) {

			readDoneCB = readDone;
			writeDoneCB = writeDone;

			// save the functors passed to us and register our own functors (which will in turn call these saved functors)
			typedef Callback<StatsDRAMSimInterface, void, unsigned, uint64_t, uint64_t> StatsDoneCB;
			StatsDoneCB *statsReadDoneCB = new StatsDoneCB(this, &StatsDRAMSimInterface::readCallback); 
			StatsDoneCB *statsWriteDoneCB = new StatsDoneCB(this, &StatsDRAMSimInterface::writeCallback); 
			mem->registerCallbacks(statsReadDoneCB, statsWriteDoneCB, reportPower);
		}

		// intercept targets for the callbacks
		void readCallback(unsigned id, uint64_t addr, uint64_t currentClockCycle) {
			stats.finishedReads++;
			assert(readDoneCB);
			(*readDoneCB)(id, addr, currentClockCycle); 
		}
		void writeCallback(unsigned id, uint64_t addr, uint64_t currentClockCycle) {
			stats.finishedWrites++;
			assert(writeDoneCB);
			(*writeDoneCB)(id, addr, currentClockCycle);
		}
	};


} //namespace 
