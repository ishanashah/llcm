#pragma once

#define _GNU_SOURCE

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

/* public */

void thread_perf_mode_init(int cpu);
void thread_perf_mode_uninit();

/* private */

void set_affinity_(int cpu) {
    cpu++;
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

void unset_realtime_priority_() {
    struct sched_param param;
    param.sched_priority = sched_get_priority_min(SCHED_OTHER);

    int ret = pthread_setschedparam(pthread_self(), SCHED_OTHER, &param);
    if (ret != 0) {
        perror("pthread_setschedparam");
        exit(1);
    }
}

void thread_perf_mode_init(int cpu) {
    set_affinity_(cpu);
    set_realtime_priority_();
}

void thread_perf_mode_uninit() { unset_realtime_priority_(); }