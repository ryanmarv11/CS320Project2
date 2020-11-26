all: clean cache-sim

cache-sim: cache-sim.o 
	g++ -o cache-sim cache-sim.cpp

clean:
	rm -rf cache-sim cache-sim.o
