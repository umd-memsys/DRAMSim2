DRAMSIM_DIR=$(PWD)/../
GTEST_PREFIX=${HOME}/software
CXXFLAGS=-I${DRAMSIM_DIR}/include -I${GTEST_PREFIX}/include -g -Wall
LDFLAGS=-L$(DRAMSIM_DIR)/lib -ldramsim -lgtest -pthread -Wl,-rpath,$(DRAMSIM_DIR)/lib -L${GTEST_PREFIX}/lib -Wl,-rpath,${GTEST_PREFIX}/lib 



SRC = $(wildcard *.cpp)
OBJ = $(addsuffix .o, $(basename $(SRC)))


RunTests: $(OBJ) 
	$(MAKE) -C .. install
	$(CXX) -o $@ $^ $(LDFLAGS)

../libdramsim.so:
	$(MAKE) -C .. 

refresh: 
	$(MAKE) -C .. clean
	$(MAKE) -C ..
	$(MAKE) clean
	$(MAKE) 

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -o $@ -c $<

clean:
	-rm *.o RunTests
