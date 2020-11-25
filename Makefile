all: clean cache-sim

cache-sim: cache-sim.o 
	gcc -o cache-sim cache-sim.c

clean:
	rm -rf cache-sim cache-sim.o
