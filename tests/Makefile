DRAMSIM_DIR=$(PWD)/../
GTEST_PREFIX=${HOME}/software
CXXFLAGS=-I${DRAMSIM_DIR}/include -I${GTEST_PREFIX}/include -g -Wall
LDFLAGS=-L$(DRAMSIM_DIR)/build/lib -ldramsim -lgtest -pthread -Wl,-rpath,$(DRAMSIM_DIR)/build/lib -L${GTEST_PREFIX}/lib -Wl,-rpath,${GTEST_PREFIX}/lib 



SRC = CSVWriter.cpp RunTests.cpp
OBJ = $(addsuffix .o, $(basename $(SRC)))


RunTests: $(OBJ) 
	$(MAKE) -C ../build install
	$(CXX) -o $@ $^ $(LDFLAGS)

#../build/libdramsim.so:
#	$(MAKE) -C ../build/ 

refresh: 
	$(MAKE) -C .. clean
	$(MAKE) -C ..
	$(MAKE) clean
	$(MAKE) 

CSVWriter.o: ../CSVWriter.h

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -o $@ -c $<

clean:
	-rm *.o RunTests
