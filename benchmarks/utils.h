#pragma once

#include <stdbool.h>
#define _GNU_SOURCE

#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

/* public */

void thread_perf_mode_main_thread_init();
void thread_perf_mode_init(int cpu);

uint64_t rdtsc(void);
uint64_t diff_timespec(const struct timespec *, const struct timespec *);

/* private */

void set_affinity_(int cpu) {
    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    CPU_SET(cpu, &cpuset);

    pthread_t current_thread = pthread_self();
    int ret = pthread_setaffinity_np(current_thread, sizeof(cpu_set_t), &cpuset);
    if (ret != 0) {
        perror("pthread_setaffinity_np");
        exit(1);
    }
}

void set_realtime_priority_() {
    struct sched_param param;
    param.sched_priority = sched_get_priority_max(SCHED_FIFO);

    int ret = pthread_setschedparam(pthread_self(), SCHED_FIFO, &param);
    if (ret != 0) {
        perror("pthread_setschedparam");
        exit(1);
    }
}

void thread_perf_mode_main_thread_init() { set_affinity_(1); }

void thread_perf_mode_init(int cpu) {
    cpu = (cpu + 1) * 2;   // main thread starts at 1, avoid hyperthreading
    set_affinity_(cpu);
    set_realtime_priority_();
}

uint64_t rdtsc(void) {
    unsigned int lo, hi;
    __asm__ __volatile__("rdtsc" : "=a"(lo), "=d"(hi));
    return ((uint64_t) hi << 32) | lo;
}

uint64_t diff_timespec(const struct timespec *time1, const struct timespec *time0) {
    return (time1->tv_sec - time0->tv_sec) * 1000000000.0 + (time1->tv_nsec - time0->tv_nsec);
}
