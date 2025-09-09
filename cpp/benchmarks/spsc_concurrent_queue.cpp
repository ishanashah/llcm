#include "lib/spsc_concurrent_queue.hpp"
#include "benchmarks/utils.hpp"

#include <optional>
#include <pthread.h>
#include <stdio.h>
#include <time.h>

static constexpr size_t MAX_SEQUENCE = 100000UL;
static constexpr size_t NUM_TESTS = 10;
static constexpr uint64_t DUMMY_ELEMENT = 1;
static constexpr size_t WARMUP_AND_WINDDOWN = MAX_SEQUENCE * 2 + 100000UL;

struct test_config {
    size_t num_threads;
    size_t num_elements;
};

struct test_result {
    uint64_t cycles;
    uint64_t nanos;
};

struct thread_args {
    SpscConcurrentQueue<uint64_t> *queue;
    struct test_config const *config;
    uint64_t *num_threads_ready;
    uint64_t const *start_barrier;
    size_t tid;
    struct test_result result;
};

void *thread_exec(void *arg0) {
    // init
    struct thread_args *thread_args = reinterpret_cast<struct thread_args *>(arg0);
    thread_perf_mode_init(thread_args->tid);
    SpscConcurrentQueue<uint64_t> *queue = thread_args->queue;
    uint64_t const *start_barrier = thread_args->start_barrier;
    auto cycle_start = rdtsc();
    auto cycle_end = rdtsc();
    struct timespec ts_start;
    struct timespec ts_end;

    __atomic_fetch_add(thread_args->num_threads_ready, 1, __ATOMIC_SEQ_CST);
    while (__atomic_load_n(start_barrier, __ATOMIC_SEQ_CST) == 0) {
    }

    if (thread_args->tid == 0) {
        // benchmark
        for (size_t i = 0; i < WARMUP_AND_WINDDOWN; i++) {
            while (!queue->TryPop())
                ;
        }
        __asm__ __volatile__("" ::: "memory");
        clock_gettime(CLOCK_MONOTONIC, &ts_start);
        cycle_start = rdtsc();
        __asm__ __volatile__("" ::: "memory");
        for (size_t i = 0; i < MAX_SEQUENCE; i++) {
            while (!queue->TryPop())
                ;
        }
        __asm__ __volatile__("" ::: "memory");
        cycle_end = rdtsc();
        clock_gettime(CLOCK_MONOTONIC, &ts_end);
        __asm__ __volatile__("" ::: "memory");
        for (size_t i = 0; i < WARMUP_AND_WINDDOWN; i++) {
            while (!queue->TryPop())
                ;
        }
    } else {
        // benchmark
        for (size_t i = 0; i < WARMUP_AND_WINDDOWN; i++) {
            while (!queue->Push(i))
                ;
        }
        __asm__ __volatile__("" ::: "memory");
        clock_gettime(CLOCK_MONOTONIC, &ts_start);
        cycle_start = rdtsc();
        __asm__ __volatile__("" ::: "memory");
        for (size_t i = 0; i < MAX_SEQUENCE; i++) {
            while (!queue->Push(i))
                ;
        }
        __asm__ __volatile__("" ::: "memory");
        cycle_end = rdtsc();
        clock_gettime(CLOCK_MONOTONIC, &ts_end);
        __asm__ __volatile__("" ::: "memory");
        for (size_t i = 0; i < WARMUP_AND_WINDDOWN; i++) {
            while (!queue->Push(i))
                ;
        }
    }

    thread_args->result = (struct test_result){.cycles = cycle_end - cycle_start,
                                               .nanos = diff_timespec(&ts_end, &ts_start)};
    return NULL;
}

struct test_result multithreaded_test(struct test_config config) {
    // init
    alignas(CACHE_LINE_SIZE) uint64_t num_threads_ready = 0;
    alignas(CACHE_LINE_SIZE) uint64_t start_barrier = 0;
    SpscConcurrentQueue<uint64_t> queue(config.num_elements);
    for (size_t i = 0; i < config.num_elements; i++) {
        queue.Push(DUMMY_ELEMENT);
    }

    // spin up threads
    pthread_t threads[config.num_threads];
    struct thread_args thread_args[config.num_threads];
    for (size_t tid = 0; tid < config.num_threads; tid++) {
        thread_args[tid] = (struct thread_args){
            .queue = &queue,
            .config = &config,
            .num_threads_ready = &num_threads_ready,
            .start_barrier = &start_barrier,
            .tid = tid,
            .result = {.cycles = 0, .nanos = 0},
        };
        int rc = pthread_create(&threads[tid], NULL, thread_exec, &thread_args[tid]);
        if (rc != 0) {
            exit(1);
        }
    }
    __atomic_fetch_add(&num_threads_ready, 1, __ATOMIC_SEQ_CST);
    while (__atomic_load_n(&num_threads_ready, __ATOMIC_SEQ_CST) != config.num_threads + 1) {
    }
    __atomic_store_n(&start_barrier, 1, __ATOMIC_SEQ_CST);

    // teardown
    for (size_t tid = 0; tid < config.num_threads; tid++) {
        pthread_join(threads[tid], NULL);
    }

    struct test_result result = {.cycles = 0, .nanos = 0};
    for (size_t tid = 0; tid < config.num_threads; tid++) {
        result.cycles += thread_args[tid].result.cycles;
        result.nanos += thread_args[tid].result.nanos;
    }
    return result;
}

void aggregate_test(struct test_config config) {
    struct test_result total = {
        .cycles = 0,
        .nanos = 0,
    };
    for (size_t i = 0; i < NUM_TESTS; i++) {
        struct test_result const current_test_result = multithreaded_test(config);
        total.cycles += current_test_result.cycles;
        total.nanos += current_test_result.nanos;
    }
    double const cycles_per_iteration =
        (double) total.cycles / (NUM_TESTS * MAX_SEQUENCE * config.num_threads);
    double const nanos_per_iteration =
        (double) total.nanos / (NUM_TESTS * MAX_SEQUENCE * config.num_threads);
    printf("threads(%lu) elements(%lu) took cycles(%lf) nanos(%lf)\n", config.num_threads,
           config.num_elements, cycles_per_iteration, nanos_per_iteration);
}

int main() {
    thread_perf_mode_main_thread_init();
    printf("running with iterations(%lu)\n", MAX_SEQUENCE);
    for (size_t num_threads = 2; num_threads <= 2; num_threads++) {
        for (size_t num_elements = 1; num_elements <= num_threads * 64; num_elements *= 2) {
            aggregate_test(
                (struct test_config){.num_threads = num_threads, .num_elements = num_elements});
        }
    }
}