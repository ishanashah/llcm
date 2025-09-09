#include "benchmarks/utils.hpp"
#include "lib/fibers/buffered_channel.hpp"
#include "lib/fibers/condition_variable.hpp"
#include "lib/fibers/fiber.hpp"
#include "lib/fibers/mutex.hpp"
#include "lib/fibers/scheduler.hpp"
#include "lib/fibers/semaphore.hpp"
#include "lib/fibers/traits.hpp"
#include "lib/fibers/unbuffered_channel.hpp"

#include <cassert>
#include <iostream>
#include <optional>
#include <stdio.h>
#include <thread>

static constexpr size_t MAX_SEQUENCE = 100000UL;
static constexpr size_t NUM_TESTS = 10;
static constexpr size_t WARMUP_AND_WINDDOWN = MAX_SEQUENCE * 2 + 100000UL;
static constexpr size_t NUM_THREADS = 1;
static constexpr size_t NUM_FIBERS = 16;
static constexpr size_t SCHEDULER_CAPACITY = 100;
static constexpr size_t STACK_SIZE = 1024 * 16;

struct TestResult {
    uint64_t cycles = 0;
    uint64_t nanos = 0;
};

struct ThreadArgs {
    size_t tid_ = 0;
    Scheduler<Traits> *scheduler = nullptr;
    uint64_t *num_threads_ready;
    uint64_t const *start_barrier;
    TestResult *result;
};

void thread_function(std::stop_token stoken, ThreadArgs args) {
    thread_perf_mode_init(args.tid_);
    Scheduler<Traits> *scheduler = args.scheduler;
    uint64_t const *start_barrier = args.start_barrier;
    __atomic_fetch_add(args.num_threads_ready, 1, __ATOMIC_SEQ_CST);
    while (__atomic_load_n(start_barrier, __ATOMIC_SEQ_CST) == 0) {
    }

    for (size_t i = 0; i < WARMUP_AND_WINDDOWN; i++) {
        bool success = false;
        do {
            success = scheduler->Poll();
        } while (!success);
    }

    __asm__ __volatile__("" ::: "memory");
    struct timespec ts_start;
    clock_gettime(CLOCK_MONOTONIC, &ts_start);
    uint64_t const cycle_start = rdtsc();

    for (size_t i = 0; i < MAX_SEQUENCE; i++) {
        bool success = false;
        do {
            success = scheduler->Poll();
        } while (!success);
    }

    __asm__ __volatile__("" ::: "memory");
    uint64_t const cycle_end = rdtsc();
    struct timespec ts_end;
    clock_gettime(CLOCK_MONOTONIC, &ts_end);
    __asm__ __volatile__("" ::: "memory");

    for (size_t i = 0; i < WARMUP_AND_WINDDOWN; i++) {
        bool success = false;
        do {
            success = scheduler->Poll();
        } while (!success);
    }

    *args.result =
        TestResult{.cycles = cycle_end - cycle_start, .nanos = diff_timespec(&ts_end, &ts_start)};
}

int main() {
    thread_perf_mode_main_thread_init();
    alignas(CACHE_LINE_SIZE) uint64_t num_threads_ready = 0;
    alignas(CACHE_LINE_SIZE) uint64_t start_barrier = 0;

    Scheduler<Traits> scheduler(SCHEDULER_CAPACITY, STACK_SIZE);
    auto const lambda = [&](Fiber<Traits> *fiber) {
        while (true) {
            fiber->Yeild();
        }
    };
    for (size_t i = 0; i < NUM_FIBERS; i++) {
        scheduler.TryCreateFiber(lambda);
    }

    TestResult results[NUM_THREADS];
    ThreadArgs args[NUM_THREADS];
    for (size_t i = 0; i < NUM_THREADS; i++) {
        args[i] = ThreadArgs{
            .tid_ = i,
            .scheduler = &scheduler,
            .num_threads_ready = &num_threads_ready,
            .start_barrier = &start_barrier,
            .result = &results[i],
        };
    }

    {
        std::jthread threads[NUM_THREADS];
        for (size_t i = 0; i < NUM_THREADS; i++) {
            threads[i] = std::jthread(thread_function, args[i]);
        }
        __atomic_fetch_add(&num_threads_ready, 1, __ATOMIC_SEQ_CST);
        while (__atomic_load_n(&num_threads_ready, __ATOMIC_SEQ_CST) != NUM_THREADS + 1) {
        }
        __atomic_store_n(&start_barrier, 1, __ATOMIC_SEQ_CST);
    }

    for (size_t i = 0; i < NUM_THREADS; i++) {
        std::cout << "THREAD " << i << " CYCLES: " << ((double) results[i].cycles) / MAX_SEQUENCE
                  << " NANOS: " << ((double) results[i].nanos / MAX_SEQUENCE) << std::endl;
    }
}