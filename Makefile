CXX = gcc
CFLAGS = -std=c11 -ggdb -Wall -O0 -I .

all: concurrent_queue_test

tests: concurrent_queue_test

concurrent_queue_test tests/concurrent_queue_test.c lib/concurrent_queue.h:
	$(CXX) $(CFLAGS) -o $@ tests/concurrent_queue_test.c

clean:
	rm -rf concurrent_queue_test