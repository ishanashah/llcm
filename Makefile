CXX = gcc
CTESTFLAGS = -pthread -std=c11 -ggdb -Wall -O0 -I .
CPERFFLAGS = -pthread -std=c11 -Wall -O3 -march=native -mtune=native -I .

all: tests benchmarks

tests: concurrent_queue_test

benchmarks: pop_and_push_benchmark

concurrent_queue_test tests/concurrent_queue_test.c:
	$(CXX) $(CTESTFLAGS) -o $@ tests/concurrent_queue_test.c

pop_and_push_benchmark benchmarks/pop_and_push.c:
	$(CXX) $(CPERFFLAGS) -o $@ benchmarks/pop_and_push.c

clean:
	rm -rf concurrent_queue_test pop_and_push_benchmark
