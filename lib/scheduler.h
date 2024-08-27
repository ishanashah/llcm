#pragma once

#include "lib/concurrent_queue.h"
#include "lib/exec_handle.h"
#include "lib/routine.h"

/* public */

struct llcm_scheduler {
    struct llcm_concurrent_queue queue;
};

void llcm_scheduler_init(struct llcm_scheduler *, size_t capacity);
void llcm_scheduler_init_with_custom_allocate(struct llcm_scheduler *, size_t capacity,
                                              struct llcm_allocator);
void llcm_scheduler_uninit(struct llcm_scheduler *);

bool llcm_scheduler_try_schedule_routine(struct llcm_scheduler *, struct llcm_routine *);
bool llcm_scheduler_poll(struct llcm_scheduler *, void *user_exec_arg);

/* private */

// should only be called by llcm_exec_handle
bool llcm_scheduler_try_reserve_new_routine_(struct llcm_scheduler *);
void llcm_scheduler_old_routine_available_(struct llcm_scheduler *);

void llcm_scheduler_init(struct llcm_scheduler *scheduler, size_t capacity) {
    llcm_scheduler_init_with_custom_allocate(scheduler, capacity, llcm_allocator_create_default());
}

void llcm_scheduler_init_with_custom_allocate(struct llcm_scheduler *scheduler, size_t capacity,
                                              struct llcm_allocator allocator) {
    llcm_concurrent_queue_init_with_custom_allocate(&scheduler->queue, capacity, allocator);
}

void llcm_scheduler_uninit(struct llcm_scheduler *scheduler) {
    llcm_concurrent_queue_uninit(&scheduler->queue);
}

bool llcm_scheduler_try_schedule_routine(struct llcm_scheduler *scheduler,
                                         struct llcm_routine *routine) {
    if (!llcm_scheduler_try_reserve_new_routine_(scheduler)) {
        return false;
    }
    llcm_concurrent_queue_push(&scheduler->queue, routine);
    return true;
}

bool llcm_scheduler_poll(struct llcm_scheduler *scheduler, void *user_exec_arg) {
    struct llcm_routine *routine = llcm_concurrent_queue_try_pop(&scheduler->queue);
    if (NULL == routine) {
        return false;
    }

    struct llcm_exec_handle exec_handle = {
        .routine = routine, .scheduler = scheduler, .user_exec_arg = user_exec_arg};
    routine->poll(routine->arg0, &exec_handle);
    if (NULL != exec_handle.scheduler) {
        llcm_concurrent_queue_push(&exec_handle.scheduler->queue, exec_handle.routine);
    }
    return true;
}

bool llcm_scheduler_try_reserve_new_routine_(struct llcm_scheduler *scheduler) {
    return llcm_concurrent_queue_try_reserve_size_before_push(&scheduler->queue, 1);
}

void llcm_scheduler_old_routine_available_(struct llcm_scheduler *scheduler) {
    llcm_concurrent_queue_unreserve_size_after_pop(&scheduler->queue, 1);
}
