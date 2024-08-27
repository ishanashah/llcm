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

    alignas(LLCM_CONCURRENT_QUEUE_CACHE_LINE_SIZE) uint64_t read_counter;
    alignas(LLCM_CONCURRENT_QUEUE_CACHE_LINE_SIZE) uint64_t write_counter;
    alignas(LLCM_CONCURRENT_QUEUE_CACHE_LINE_SIZE) uint64_t reserved_push_size;
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
    alignas(LLCM_CONCURRENT_QUEUE_CACHE_LINE_SIZE) volatile uint64_t aba_counter;
    void *element;
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
    if (capacity < 2) {
        capacity = 2;   // capacity must be at least 2 for aba_counter
    }
    size_t const array_size_bytes = sizeof(struct llcm_concurrent_queue_entry) * capacity;
    queue->array = (struct llcm_concurrent_queue_entry *) allocator.allocate(
        sizeof(struct llcm_concurrent_queue_entry), array_size_bytes);
    memset(queue->array, 0, array_size_bytes);
    for (uint64_t i = 0; i < capacity; i++) {
        queue->array[i].aba_counter = i;
    }
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
    uint64_t const reserved_write_counter =
        __atomic_fetch_add(&queue->write_counter, 1, __ATOMIC_SEQ_CST);
    struct llcm_concurrent_queue_entry *entry = &queue->array[reserved_write_counter & queue->mask];
    while (entry->aba_counter != reserved_write_counter) {
    }
    entry->element = value;
    __asm__ __volatile__("" ::: "memory");
    entry->aba_counter = reserved_write_counter + 1;
}

void *llcm_concurrent_queue_try_pop(struct llcm_concurrent_queue *queue) {
    uint64_t const local_write_counter = queue->write_counter;
    uint64_t *read_ptr = &queue->read_counter;
    uint64_t local_read_counter = *read_ptr;
    while (local_read_counter < local_write_counter) {
        if (__atomic_compare_exchange_n(read_ptr, &local_read_counter, local_read_counter + 1,
                                        false, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST)) {
            struct llcm_concurrent_queue_entry *entry =
                &queue->array[local_read_counter & queue->mask];
            while (entry->aba_counter != local_read_counter + 1) {
            }
            void *read_value = entry->element;
            __asm__ __volatile__("" ::: "memory");
            entry->aba_counter = local_read_counter + queue->mask + 1;
            return read_value;
        }
    }
    return NULL;
}