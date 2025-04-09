#pragma once

#include "lib/routine.h"
#include <stdbool.h>
#include <stddef.h>

/* public */

struct llcm_routine;
struct llcm_scheduler;

struct llcm_exec_handle {
    struct llcm_routine *routine;
    struct llcm_scheduler *scheduler;
    void *user_exec_arg;
};

struct llcm_routine *llcm_exec_handle_get_current_routine(struct llcm_exec_handle *);
void llcm_exec_handle_set_routine(struct llcm_exec_handle *handle, struct llcm_routine *routine);
void llcm_exec_handle_cancel_routine(struct llcm_exec_handle *);
struct llcm_scheduler *llcm_exec_handle_get_current_scheduler(struct llcm_exec_handle *);
bool llcm_exec_handle_try_switch_scheduler(struct llcm_exec_handle *, struct llcm_scheduler *);

/* private */

// defined in scheduler.h
bool llcm_scheduler_try_reserve_new_routine_(struct llcm_scheduler *);
void llcm_scheduler_old_routine_available_(struct llcm_scheduler *);

struct llcm_routine *llcm_exec_handle_get_current_routine(struct llcm_exec_handle *handle) {
    return handle->routine;
}

void llcm_exec_handle_set_routine(struct llcm_exec_handle *handle, struct llcm_routine *routine) {
    handle->routine = routine;
}

void llcm_exec_handle_cancel_routine(struct llcm_exec_handle *handle) {
    if (NULL != handle->scheduler) {
        llcm_scheduler_old_routine_available_(handle->scheduler);
        handle->scheduler = NULL;
    }
}

struct llcm_scheduler *llcm_exec_handle_get_current_scheduler(struct llcm_exec_handle *handle) {
    return handle->scheduler;
}

bool llcm_exec_handle_try_switch_scheduler(struct llcm_exec_handle *handle,
                                           struct llcm_scheduler *new_scheduler) {
    if (NULL == new_scheduler || handle->scheduler == new_scheduler) {
        return false;
    }
    if (!llcm_scheduler_try_reserve_new_routine_(new_scheduler)) {
        return false;
    }
    llcm_scheduler_old_routine_available_(handle->scheduler);
    handle->scheduler = new_scheduler;
    return true;
}