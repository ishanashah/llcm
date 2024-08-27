#pragma once
#include <stdlib.h>

/* public */

struct llcm_allocator {
    void *(*allocate)(size_t alignment, size_t size);
    void (*free)(void *);
};

struct llcm_allocator llcm_allocator_create_default(void) {
    return (struct llcm_allocator){.allocate = aligned_alloc, .free = free};
}