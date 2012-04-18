CXX=g++
CXXFLAGS=-I/home/prosenf1/bin/include 

SRC = $(wildcard *.cpp)
OBJ = $(addsuffix .o, $(basename $(SRC)))

ALL: $(OBJ)

%.o: %.cpp
	g++ $(CXXFLAGS) -o $@ -c $< 

