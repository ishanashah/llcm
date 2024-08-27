#define _GNU_SOURCE

#include "lib/concurrent_queue.h"
#include "benchmarks/utils.h"

#include <pthread.h>
#include <stdio.h>
#include <time.h>

#define MAX_SEQUENCE  1000000UL
#define NUM_TESTS     5
#define DUMMY_ELEMENT ((void *) 1)

struct test_config {
    size_t num_elements;
    size_t num_threads;
};

struct thread_args {
    struct llcm_concurrent_queue *queue;
    struct test_config const *config;
    uint64_t *push_counter;
    uint64_t *start_barrier;
};

void *thread_exec(void *arg0) {
    struct thread_args *thread_args = arg0;
    struct llcm_concurrent_queue *queue = thread_args->queue;
    uint64_t *push_counter = thread_args->push_counter;
    uint64_t tid = __atomic_fetch_add(thread_args->start_barrier, 1, __ATOMIC_SEQ_CST);
    thread_perf_mode_init(tid);
    while (__atomic_load_n(thread_args->start_barrier, __ATOMIC_SEQ_CST) !=
           thread_args->config->num_threads + 1) {
    }

    for (; __atomic_load_n(push_counter, __ATOMIC_SEQ_CST) < MAX_SEQUENCE;
         __atomic_fetch_add(push_counter, 1, __ATOMIC_SEQ_CST)) {
        void *pop_result = NULL;
        while (NULL == pop_result) {
            pop_result = llcm_concurrent_queue_try_pop(queue);
        }
        llcm_concurrent_queue_push(queue, pop_result);
    }
    thread_perf_mode_uninit();
    return NULL;
}

uint64_t multithreaded_test(struct test_config config) {
    struct llcm_concurrent_queue queue;
    alignas(LLCM_CONCURRENT_QUEUE_CACHE_LINE_SIZE) uint64_t push_counter = 0;
    alignas(LLCM_CONCURRENT_QUEUE_CACHE_LINE_SIZE) uint64_t start_barrier = 0;
    llcm_concurrent_queue_init(&queue, config.num_elements);
    llcm_concurrent_queue_try_reserve_size_before_push(&queue, config.num_elements);
    for (size_t i = 0; i < config.num_elements; i++) {
        llcm_concurrent_queue_push(&queue, DUMMY_ELEMENT);
    }

    struct thread_args thread_args = {.queue = &queue,
                                      .config = &config,
                                      .push_counter = &push_counter,
                                      .start_barrier = &start_barrier};
    pthread_t threads[config.num_threads];
    for (size_t tid = 0; tid < config.num_threads; tid++) {
        int rc = pthread_create(&threads[tid], NULL, thread_exec, &thread_args);
        if (rc != 0) {
            exit(1);
        }
    }
    uint64_t tid = __atomic_fetch_add(&start_barrier, 1, __ATOMIC_SEQ_CST);
    thread_perf_mode_init(tid);
    while (__atomic_load_n(&start_barrier, __ATOMIC_SEQ_CST) != config.num_threads + 1) {
    }

    uint64_t const start_time = rdtsc();
    while (__atomic_load_n(&push_counter, __ATOMIC_SEQ_CST) < MAX_SEQUENCE) {
    }
    uint64_t const end_time = rdtsc();

    for (size_t tid = 0; tid < config.num_threads; tid++) {
        pthread_join(threads[tid], NULL);
    }

    llcm_concurrent_queue_unreserve_size_after_pop(&queue, config.num_elements);
    llcm_concurrent_queue_uninit(&queue);
    thread_perf_mode_uninit();
    return end_time - start_time;
}

void aggregate_test(struct test_config config) {
    uint64_t total_cycles = 0;
    for (int i = 0; i < NUM_TESTS; i++) {
        total_cycles += multithreaded_test(config);
    }
    uint64_t const average_cycles = total_cycles / NUM_TESTS;
    double const cycles_per_iteration = average_cycles / (double) MAX_SEQUENCE;
    printf(
        "iterations(%lu) elements(%lu) threads(%lu) took cycles(%lu), cycles_per_iteration(%lf)\n",
        MAX_SEQUENCE, config.num_elements, config.num_threads, average_cycles,
        cycles_per_iteration);
}

int main() {
    aggregate_test((struct test_config){.num_elements = 1, .num_threads = 1});
    aggregate_test((struct test_config){.num_elements = 1, .num_threads = 2});
    aggregate_test((struct test_config){.num_elements = 1, .num_threads = 4});
    aggregate_test((struct test_config){.num_elements = 2, .num_threads = 1});
    aggregate_test((struct test_config){.num_elements = 2, .num_threads = 2});
    aggregate_test((struct test_config){.num_elements = 2, .num_threads = 4});
    aggregate_test((struct test_config){.num_elements = 4, .num_threads = 1});
    aggregate_test((struct test_config){.num_elements = 4, .num_threads = 2});
    aggregate_test((struct test_config){.num_elements = 4, .num_threads = 4});
}