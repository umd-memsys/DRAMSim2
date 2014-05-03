cmake_minimum_required(VERSION 2.8)
project(DRAMSim2)

IF(NOT CMAKE_BUILD_TYPE)
	SET(CMAKE_BUILD_TYPE "Release")
ENDIF()

set (CMAKE_CXX_FLAGS_RELEASE "-g -O3 -march=native -DLOG_OUTPUT")
set (CMAKE_CXX_FLAGS_DEBUG "-g -O0 -DLOG_OUTPUT")
set (CMAKE_EXPORT_COMPILE_COMMANDS TRUE)

add_library ( dramsimutils SHARED Util.cpp util.cpp ClockDomain.cpp )
add_library ( dramsim SHARED AddressMapping.cpp Bank.cpp BankState.cpp BusPacket.cpp
	CommandQueue.cpp IniReader.cpp MemoryController.cpp
	MemorySystem.cpp MultiChannelMemorySystem.cpp Rank.cpp SimulatorObject.cpp
	Transaction.cpp )
add_executable ( DRAMSim TraceBasedSim.cpp )
target_link_libraries( dramsim dramsimutils )
target_link_libraries( DRAMSim dramsimutils dramsim )

INSTALL( TARGETS dramsim DESTINATION lib COMPONENT library)
INSTALL( TARGETS dramsimutils DESTINATION lib COMPONENT library)
INSTALL( FILES Callback.h CSVWriter.h DRAMSim.h DESTINATION include COMPONENT headers ) 
