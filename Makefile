objects=tsp.o main.o prallelTsp.o
CC=gcc
CXX=g++
LINKFLAGS=-lpthread -g 
CXXFLAGS=-c -g
tsp:$(objects)
	$(CXX) -o tsp $(objects) $(LINKFLAGS)
	rm -f $(objects)
%.o:%.cc
	$(CXX) $(CXXFLAGS) $<
clean:
	rm -f tsp $(objects)