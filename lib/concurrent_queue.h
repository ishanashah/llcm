#pragma once

#include "lib/allocator.h"
#include "lib/utils.h"

#include <assert.h>
#include <stdalign.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

/* public */

#define LLCM_CONCURRENT_QUEUE_CACHE_LINE_SIZE 64

struct llcm_concurrent_queue_entry;

struct llcm_concurrent_queue {
    struct llcm_concurrent_queue_entry *array;
    size_t mask;
    void (*free)(void *);

    alignas(LLCM_CONCURRENT_QUEUE_CACHE_LINE_SIZE) int64_t available_pop_size;
    alignas(LLCM_CONCURRENT_QUEUE_CACHE_LINE_SIZE) uint64_t reserved_push_size;
    alignas(LLCM_CONCURRENT_QUEUE_CACHE_LINE_SIZE) uint64_t read_counter;
    alignas(LLCM_CONCURRENT_QUEUE_CACHE_LINE_SIZE) uint64_t write_counter;
};

void llcm_concurrent_queue_init(struct llcm_concurrent_queue *, size_t capacity);
void llcm_concurrent_queue_init_with_custom_allocate(struct llcm_concurrent_queue *,
                                                     size_t capacity, struct llcm_allocator);
void llcm_concurrent_queue_uninit(struct llcm_concurrent_queue *);

size_t llcm_concurrent_queue_get_capacity(struct llcm_concurrent_queue const *);

// must reserve available capacity for any new entries to be pushed
bool llcm_concurrent_queue_try_reserve_size_before_push(struct llcm_concurrent_queue *,
                                                        size_t num_new_entries);
// can be called after an entry is popped and will not be pushed again
void llcm_concurrent_queue_unreserve_size_after_pop(struct llcm_concurrent_queue *,
                                                    size_t num_old_entries);

// does not block and always succeeds
void llcm_concurrent_queue_push(struct llcm_concurrent_queue *, void *value);
// returns NULL on failure
void *llcm_concurrent_queue_try_pop(struct llcm_concurrent_queue *);

/* private */

struct llcm_concurrent_queue_entry {
    alignas(LLCM_CONCURRENT_QUEUE_CACHE_LINE_SIZE) void *entry;
};
static_assert(sizeof(struct llcm_concurrent_queue_entry) == LLCM_CONCURRENT_QUEUE_CACHE_LINE_SIZE,
              "");

size_t llcm_concurrent_queue_get_capacity(struct llcm_concurrent_queue const *queue) {
    return queue->mask + 1;
}

void llcm_concurrent_queue_init(struct llcm_concurrent_queue *queue, size_t capacity) {
    llcm_concurrent_queue_init_with_custom_allocate(queue, capacity,
                                                    llcm_allocator_create_default());
}

void llcm_concurrent_queue_init_with_custom_allocate(struct llcm_concurrent_queue *queue,
                                                     size_t capacity,
                                                     struct llcm_allocator allocator) {
    memset(queue, 0, sizeof(*queue));
    capacity = llcm_round_up_pow2(capacity);
    queue->array = (struct llcm_concurrent_queue_entry *) allocator.allocate(
        sizeof(struct llcm_concurrent_queue_entry),
        sizeof(struct llcm_concurrent_queue_entry) * capacity);
    queue->mask = capacity - 1;
    queue->free = allocator.free;
}

void llcm_concurrent_queue_uninit(struct llcm_concurrent_queue *queue) {
    queue->free(queue->array);
}

bool llcm_concurrent_queue_try_reserve_size_before_push(struct llcm_concurrent_queue *queue,
                                                        size_t num_new_entries) {
    uint64_t const reserved_push_size =
        __atomic_fetch_add(&queue->reserved_push_size, num_new_entries, __ATOMIC_SEQ_CST);
    if (reserved_push_size > queue->mask) {
        __atomic_fetch_sub(&queue->reserved_push_size, num_new_entries, __ATOMIC_SEQ_CST);
        return false;
    }
    return true;
}

void llcm_concurrent_queue_unreserve_size_after_pop(struct llcm_concurrent_queue *queue,
                                                    size_t num_old_entries) {
    __atomic_fetch_sub(&queue->reserved_push_size, num_old_entries, __ATOMIC_SEQ_CST);
}

void llcm_concurrent_queue_push(struct llcm_concurrent_queue *queue, void *value) {
    __atomic_fetch_add(&queue->available_pop_size, 1, __ATOMIC_SEQ_CST);

    uint64_t const reserved_write_counter =
        __atomic_fetch_add(&queue->write_counter, 1, __ATOMIC_SEQ_CST);
    struct llcm_concurrent_queue_entry *read_entry =
        &queue->array[reserved_write_counter & queue->mask];
    void *expected_value = NULL;
    while (!__atomic_compare_exchange_n(&read_entry->entry, &expected_value, value, false,
                                        __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST)) {
        expected_value = NULL;
    }
}

void *llcm_concurrent_queue_try_pop(struct llcm_concurrent_queue *queue) {
    int64_t const available_pop_size =
        __atomic_fetch_sub(&queue->available_pop_size, 1, __ATOMIC_SEQ_CST);
    if (available_pop_size <= 0) {
        __atomic_fetch_add(&queue->available_pop_size, 1, __ATOMIC_SEQ_CST);
        return NULL;
    }

    uint64_t const reserved_read_counter =
        __atomic_fetch_add(&queue->read_counter, 1, __ATOMIC_SEQ_CST);
    struct llcm_concurrent_queue_entry *read_entry =
        &queue->array[reserved_read_counter & queue->mask];
    void *read_value = NULL;
    while (NULL == read_value) {
        read_value = __atomic_exchange_n(&read_entry->entry, NULL, __ATOMIC_SEQ_CST);
    }
    return read_value;
}