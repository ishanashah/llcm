#pragma once

#include "lib/concurrent_queue.h"

/* public */

struct llcm_scheduler {
    struct llcm_concurrent_queue queue;
};

void llcm_scheduler_init(struct llcm_scheduler *, size_t capacity);
void llcm_scheduler_init_with_custom_allocate(struct llcm_scheduler *,
                                              size_t capacity,
                                              struct llcm_allocator);
void llcm_scheduler_uninit(struct llcm_scheduler *);

/* private */

void llcm_scheduler_init(struct llcm_scheduler *scheduler, size_t capacity) {
    llcm_scheduler_init_with_custom_allocate(scheduler, capacity,
                                             llcm_allocator_create_default());
}

void llcm_scheduler_init_with_custom_allocate(struct llcm_scheduler *scheduler,
                                              size_t capacity,
                                              struct llcm_allocator allocator) {
    llcm_concurrent_queue_init_with_custom_allocate(&scheduler->queue, capacity,
                                                    allocator);
}

void llcm_scheduler_uninit(struct llcm_scheduler *scheduler) {
    llcm_concurrent_queue_uninit(&scheduler->queue);
}
