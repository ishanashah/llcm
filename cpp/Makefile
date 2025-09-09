CXX = g++
CTESTFLAGS = -pthread -std=gnu++20 -I/usr/include -ggdb -Wall -O0 -I .
CPERFFLAGS = -pthread -std=gnu++20 -I/usr/include -Wall -O3 -march=native -mtune=native -I .

# Add the library path & the boost_context link flag
BOOST_LIBS = -lboost_context
BOOST_LIBDIR = -L/usr/lib/x86_64-linux-gnu

all: tests benchmarks

tests: concurrent_queue_test dynamic_concurrent_queue_test sandbox_test

benchmarks: concurrent_queue_benchmark dynamic_concurrent_queue_benchmark context_switch_benchmark

concurrent_queue_test tests/concurrent_queue_test.cpp:
	$(CXX) $(CTESTFLAGS) $(BOOST_LIBDIR) -o $@ tests/concurrent_queue_test.cpp $(BOOST_LIBS)

dynamic_concurrent_queue_test tests/dynamic_concurrent_queue_test.cpp:
	$(CXX) $(CTESTFLAGS) $(BOOST_LIBDIR) -o $@ tests/dynamic_concurrent_queue_test.cpp $(BOOST_LIBS)

sandbox_test tests/sandbox_test.cpp:
	$(CXX) $(CTESTFLAGS) $(BOOST_LIBDIR) -o $@ tests/sandbox_test.cpp $(BOOST_LIBS)

concurrent_queue_benchmark benchmarks/concurrent_queue.cpp:
	$(CXX) $(CPERFFLAGS) $(BOOST_LIBDIR) -o $@ benchmarks/concurrent_queue.cpp $(BOOST_LIBS)

dynamic_concurrent_queue_benchmark benchmarks/dynamic_concurrent_queue.cpp:
	$(CXX) $(CPERFFLAGS) $(BOOST_LIBDIR) -o $@ benchmarks/dynamic_concurrent_queue.cpp $(BOOST_LIBS)

context_switch_benchmark benchmarks/context_switch.cpp:
	$(CXX) $(CPERFFLAGS) $(BOOST_LIBDIR) -o $@ benchmarks/context_switch.cpp $(BOOST_LIBS)

clean:
	rm -rf concurrent_queue_test dynamic_concurrent_queue_test sandbox_test concurrent_queue_benchmark dynamic_concurrent_queue_benchmark context_switch_benchmark
