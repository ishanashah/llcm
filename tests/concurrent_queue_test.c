#include "lib/concurrent_queue.h"

#include <pthread.h>
#include <stdio.h>

void basic_queue_test() {
    size_t const capacity = 16;
    struct llcm_concurrent_queue queue;
    llcm_concurrent_queue_init(&queue, capacity);
    assert(capacity == llcm_concurrent_queue_get_capacity(&queue));

    for (uint64_t i = 1; i <= capacity * 4; i++) {
        bool const reserve_result =
            llcm_concurrent_queue_try_reserve_size_before_push(&queue, 1);
        assert(reserve_result);
        llcm_concurrent_queue_push(&queue, (void *) i);
        void *pop_result = llcm_concurrent_queue_try_pop(&queue);
        assert((uint64_t) pop_result == i);
        llcm_concurrent_queue_unreserve_size_after_pop(&queue, 1);
    }

    // reserve capacity entries to push
    for (uint64_t i = 1; i <= capacity; i++) {
        bool const reserve_result =
            llcm_concurrent_queue_try_reserve_size_before_push(&queue, 1);
        assert(reserve_result);
    }

    // reserving capacity + 1th entry should fail
    {
        bool const reserve_result =
            llcm_concurrent_queue_try_reserve_size_before_push(&queue, 1);
        assert(!reserve_result);
    }

    // reserve capacity entries at once
    {
        llcm_concurrent_queue_unreserve_size_after_pop(&queue, capacity);
        bool const reserve_result =
            llcm_concurrent_queue_try_reserve_size_before_push(&queue,
                                                               capacity);
        assert(reserve_result);
    }

    for (uint64_t i = 1; i <= capacity; i++) {
        llcm_concurrent_queue_push(&queue, (void *) i);
    }
    for (uint64_t i = 1; i <= capacity; i++) {
        void *pop_result = llcm_concurrent_queue_try_pop(&queue);
        assert((uint64_t) pop_result == i);
    }
    assert(NULL == llcm_concurrent_queue_try_pop(&queue));
    llcm_concurrent_queue_unreserve_size_after_pop(&queue, capacity);

    llcm_concurrent_queue_uninit(&queue);
    printf("PASSED basic_queue_test\n");
}

#define MULTITHREADED_TEST_NUM_THREADS  32
#define MULTITHREADED_TEST_MAX_SEQUENCE 1024

struct sequence_tracker {
    uint64_t num_observations[MULTITHREADED_TEST_MAX_SEQUENCE];
};

void sequence_tracker_init(struct sequence_tracker *tracker) {
    memset(tracker, 0, sizeof(*tracker));
}

void sequence_tracker_mark_observed(struct sequence_tracker *tracker,
                                    uint64_t sequence_number) {
    __atomic_fetch_add(&tracker->num_observations[sequence_number], 1,
                       __ATOMIC_SEQ_CST);
}

void sequence_tracker_validate(struct sequence_tracker *tracker) {
    for (uint64_t sequence = 1; sequence < MULTITHREADED_TEST_MAX_SEQUENCE;
         sequence++) {
        assert(tracker->num_observations[sequence] ==
               MULTITHREADED_TEST_NUM_THREADS);
    }
}

struct thread_args {
    struct llcm_concurrent_queue *queue;
    struct sequence_tracker *sequence_tracker;
};

void *thread_exec(void *arg0) {
    struct thread_args *args = arg0;
    for (uint64_t sequence_number = 1;
         sequence_number < MULTITHREADED_TEST_MAX_SEQUENCE; sequence_number++) {
        llcm_concurrent_queue_push(args->queue, (void *) sequence_number);
        void *pop_result = NULL;
        while (NULL == pop_result) {
            pop_result = llcm_concurrent_queue_try_pop(args->queue);
        }
        sequence_tracker_mark_observed(args->sequence_tracker,
                                       (uint64_t) pop_result);
    }
    return NULL;
}

void multithreaded_test() {
    struct llcm_concurrent_queue queue;
    llcm_concurrent_queue_init(&queue, MULTITHREADED_TEST_NUM_THREADS);
    bool const was_reserved =
        llcm_concurrent_queue_try_reserve_size_before_push(
            &queue, MULTITHREADED_TEST_NUM_THREADS);
    assert(was_reserved);
    struct sequence_tracker sequence_tracker;
    sequence_tracker_init(&sequence_tracker);

    struct thread_args thread_args = {.queue = &queue,
                                      .sequence_tracker = &sequence_tracker};
    pthread_t threads[MULTITHREADED_TEST_NUM_THREADS];
    for (size_t tid = 0; tid < MULTITHREADED_TEST_NUM_THREADS; tid++) {
        int rc = pthread_create(&threads[tid], NULL, thread_exec, &thread_args);
        assert(rc == 0);
    }

    for (size_t tid = 0; tid < MULTITHREADED_TEST_NUM_THREADS; tid++) {
        pthread_join(threads[tid], NULL);
    }

    llcm_concurrent_queue_unreserve_size_after_pop(
        &queue, MULTITHREADED_TEST_NUM_THREADS);
    llcm_concurrent_queue_uninit(&queue);
    sequence_tracker_validate(&sequence_tracker);
    printf("PASSED multithreaded_test\n");
}

int main() {
    basic_queue_test();
    multithreaded_test();
}