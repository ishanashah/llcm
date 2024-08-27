#define _GNU_SOURCE

#include "lib/concurrent_queue.h"
#include "benchmarks/utils.h"

#include <pthread.h>
#include <stdio.h>
#include <time.h>

#define MAX_SEQUENCE  100000UL
#define NUM_TESTS     10
#define DUMMY_ELEMENT ((void *) 1)

struct test_config {
    size_t num_elements;
    size_t num_threads;
};

struct thread_args {
    struct llcm_concurrent_queue *queue;
    struct test_config const *config;
    uint64_t *num_threads_ready;
    uint64_t const *start_barrier;
    uint64_t const *end_barrier;
    int tid;
};

void *thread_exec(void *arg0) {
    // init
    struct thread_args *thread_args = arg0;
    thread_perf_mode_init(thread_args->tid + 2);
    struct llcm_concurrent_queue *queue = thread_args->queue;
    uint64_t const *start_barrier = thread_args->start_barrier;
    uint64_t const *end_barrier = thread_args->end_barrier;
    __atomic_fetch_add(thread_args->num_threads_ready, 1, __ATOMIC_SEQ_CST);
    while (__atomic_load_n(start_barrier, __ATOMIC_SEQ_CST) == 0) {
    }

    // benchmark
    while (__atomic_load_n(end_barrier, __ATOMIC_SEQ_CST) == 0) {
        void *pop_result = NULL;
        while (NULL == pop_result) {
            pop_result = llcm_concurrent_queue_try_pop(queue);
        }
        llcm_concurrent_queue_push(queue, pop_result);
    }
    thread_perf_mode_uninit();
    return NULL;
}

struct test_result {
    uint64_t cycles;
    uint64_t nanos;
};

struct test_result multithreaded_test(struct test_config config) {
    // init
    struct llcm_concurrent_queue queue;
    alignas(LLCM_CONCURRENT_QUEUE_CACHE_LINE_SIZE) uint64_t num_threads_ready = 0;
    alignas(LLCM_CONCURRENT_QUEUE_CACHE_LINE_SIZE) uint64_t start_barrier = 0;
    alignas(LLCM_CONCURRENT_QUEUE_CACHE_LINE_SIZE) uint64_t end_barrier = 0;
    llcm_concurrent_queue_init(&queue, config.num_elements);
    llcm_concurrent_queue_try_reserve_size_before_push(&queue, config.num_elements);
    for (size_t i = 0; i < config.num_elements; i++) {
        llcm_concurrent_queue_push(&queue, DUMMY_ELEMENT);
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
            .end_barrier = &end_barrier,
            .tid = tid,
        };
        int rc = pthread_create(&threads[tid], NULL, thread_exec, &thread_args[tid]);
        if (rc != 0) {
            exit(1);
        }
    }
    thread_perf_mode_init(1);
    __atomic_fetch_add(&num_threads_ready, 1, __ATOMIC_SEQ_CST);
    while (__atomic_load_n(&num_threads_ready, __ATOMIC_SEQ_CST) != config.num_threads + 1) {
    }

    // benchmark
    __asm__ __volatile__("" ::: "memory");
    struct timespec ts_start;
    clock_gettime(CLOCK_REALTIME, &ts_start);
    uint64_t const cycle_start = rdtsc();
    __asm__ __volatile__("" ::: "memory");
    __atomic_store_n(&start_barrier, 1, __ATOMIC_SEQ_CST);
    while (__atomic_load_n(&queue.write_counter, __ATOMIC_SEQ_CST) < MAX_SEQUENCE) {
    }
    __asm__ __volatile__("" ::: "memory");
    uint64_t const cycle_end = rdtsc();
    struct timespec ts_end;
    clock_gettime(CLOCK_REALTIME, &ts_end);
    __asm__ __volatile__("" ::: "memory");

    // teardown
    __atomic_store_n(&end_barrier, 1, __ATOMIC_SEQ_CST);
    for (size_t tid = 0; tid < config.num_threads; tid++) {
        pthread_join(threads[tid], NULL);
    }
    llcm_concurrent_queue_unreserve_size_after_pop(&queue, config.num_elements);
    llcm_concurrent_queue_uninit(&queue);
    thread_perf_mode_uninit();
    return (struct test_result){.cycles = cycle_end - cycle_start,
                                .nanos = diff_timespec(&ts_end, &ts_start)};
}

void aggregate_test(struct test_config config) {
    struct test_result total = {
        .cycles = 0,
        .nanos = 0,
    };
    for (int i = 0; i < NUM_TESTS; i++) {
        struct test_result const current_test_result = multithreaded_test(config);
        total.cycles += current_test_result.cycles;
        total.nanos += current_test_result.nanos;
    }
    double const cycles_per_iteration = (double) total.cycles / (NUM_TESTS * MAX_SEQUENCE);
    double const nanos_per_iteration = (double) total.nanos / (NUM_TESTS * MAX_SEQUENCE);
    printf("iterations(%lu) elements(%lu) threads(%lu) took cycles_per_iteration(%lf) "
           "nanos_per_iteration(%lf)\n",
           MAX_SEQUENCE, config.num_elements, config.num_threads, cycles_per_iteration,
           nanos_per_iteration);
}

int main() {
    aggregate_test((struct test_config){.num_elements = 1, .num_threads = 1});
    aggregate_test((struct test_config){.num_elements = 2, .num_threads = 1});
    aggregate_test((struct test_config){.num_elements = 2, .num_threads = 2});
    aggregate_test((struct test_config){.num_elements = 4, .num_threads = 1});
    aggregate_test((struct test_config){.num_elements = 4, .num_threads = 2});
    aggregate_test((struct test_config){.num_elements = 4, .num_threads = 4});
}