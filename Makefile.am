ACLOCAL_AMFLAGS = -I m4

# DRAMSim2 library 
lib_LTLIBRARIES = libdramsim.la
libdramsim_la_CPPFLAGS = -DNO_STORAGE
libdramsim_la_SOURCES = AddressMapping.cpp \
	Bank.cpp \
	BankState.cpp \
	BusPacket.cpp \
	ClockDomain.cpp \
	CommandQueue.cpp \
	IniReader.cpp \
	MemoryController.cpp \
	MemorySystem.cpp \
	MultiChannelMemorySystem.cpp \
	Rank.cpp \
	SimulatorObject.cpp \
	Transaction.cpp \
	Util.cpp

# These are the only public headers needed to use the DRAMSimInterface 
include_HEADERS = CSVWriter.h \
						DRAMSim.h \
						Callback.h 

# TraceBasedSim binary (./DRAMSim)
bin_PROGRAMS = DRAMSim
DRAMSim_CPPFLAGS = -DNO_STORAGE
DRAMSim_SOURCES = TraceBasedSim.cpp
DRAMSim_LDADD = libdramsim.la

