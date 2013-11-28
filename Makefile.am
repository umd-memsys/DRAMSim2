ACLOCAL_AMFLAGS = -I m4

GIT_BRANCH=$(shell git branch | grep '\*' | awk '{print $$2}')
GIT_VERSION=$(shell git describe)
GIT_DESCRIPTION=-DGIT_VERSION=$(GIT_VERSION)-$(GIT_BRANCH)
# DRAMSim2 library 
lib_LTLIBRARIES = libdramsim.la
libdramsim_la_CPPFLAGS = -DNO_STORAGE $(GIT_DESCRIPTION)
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
	Util.cpp \
	util.cpp

# These are the only public headers needed to use the DRAMSimInterface 
include_HEADERS = CSVWriter.h \
						DRAMSim.h \
						Callback.h 

# TraceBasedSim binary (./DRAMSim)
bin_PROGRAMS = DRAMSim
DRAMSim_CPPFLAGS = -DNO_STORAGE $(GIT_DESCRIPTION)
DRAMSim_SOURCES = TraceBasedSim.cpp
DRAMSim_LDADD = libdramsim.la

