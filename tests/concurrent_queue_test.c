#include "lib/concurrent_queue.h"

#include <stdio.h>

void basic_queue_test() {
    size_t const capacity = 16;
    struct llcm_concurrent_queue queue;
    llcm_concurrent_queue_init(&queue, 16);

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

    printf("PASSED basic_queue_test\n");
}

int main() { basic_queue_test(); }