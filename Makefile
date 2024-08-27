CXX = gcc
CTESTFLAGS = -pthread -std=c11 -ggdb -Wall -O0 -I .
CPERFFLAGS = -pthread -std=c11 -Wall -O3 -march=native -mtune=native -I .

all: tests benchmarks

tests: concurrent_queue_test

benchmarks: concurrent_queue_benchmark

concurrent_queue_test tests/concurrent_queue_test.c:
	$(CXX) $(CTESTFLAGS) -o $@ tests/concurrent_queue_test.c

concurrent_queue_benchmark benchmarks/concurrent_queue.c:
	$(CXX) $(CPERFFLAGS) -o $@ benchmarks/concurrent_queue.c

clean:
	rm -rf concurrent_queue_test concurrent_queue_benchmark
