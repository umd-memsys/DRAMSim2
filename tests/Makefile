DRAMSIM_DIR=$(PWD)/../ 
GTEST_PREFIX=${HOME}/software
CXXFLAGS=-I${DRAMSIM_DIR} -I${GTEST_PREFIX}/include -g -Wall
LDFLAGS=-L$(DRAMSIM_DIR) -ldramsim -lgtest -pthread -Wl,-rpath,$(DRAMSIM_DIR) -L${GTEST_PREFIX}/lib -Wl,-rpath,${GTEST_PREFIX}/lib 



SRC = $(wildcard *.cpp)
OBJ = $(addsuffix .o, $(basename $(SRC)))


tests: $(OBJ) 
	$(MAKE) -C .. 
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
	-rm tests *.o
