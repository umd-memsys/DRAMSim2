ACLOCAL_AMFLAGS = -I m4
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

bin_PROGRAMS = DRAMSim
DRAMSim_CPPFLAGS = -DNO_STORAGE
DRAMSim_SOURCES = TraceBasedSim.cpp
DRAMSim_LDADD = libdramsim.la
