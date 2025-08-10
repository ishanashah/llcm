CXX = g++
CTESTFLAGS = -pthread -std=c++2a -ggdb -Wall -O0 -I .
CPERFFLAGS = -pthread -std=c++2a -Wall -O3 -march=native -mtune=native -I .

all: tests benchmarks

tests: concurrent_queue_test dynamic_concurrent_queue_test

benchmarks: concurrent_queue_benchmark dynamic_concurrent_queue_benchmark

concurrent_queue_test tests/concurrent_queue_test.cpp:
	$(CXX) $(CTESTFLAGS) -o $@ tests/concurrent_queue_test.cpp

dynamic_concurrent_queue_test tests/dynamic_concurrent_queue_test.cpp:
	$(CXX) $(CTESTFLAGS) -o $@ tests/dynamic_concurrent_queue_test.cpp

concurrent_queue_benchmark benchmarks/concurrent_queue.cpp:
	$(CXX) $(CPERFFLAGS) -o $@ benchmarks/concurrent_queue.cpp

dynamic_concurrent_queue_benchmark benchmarks/dynamic_concurrent_queue.cpp:
	$(CXX) $(CPERFFLAGS) -o $@ benchmarks/dynamic_concurrent_queue.cpp

clean:
	rm -rf concurrent_queue_test dynamic_concurrent_queue_test concurrent_queue_benchmark dynamic_concurrent_queue_benchmark
