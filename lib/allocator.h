#pragma once
#include <stdlib.h>

/* public */

struct llcm_allocator {
    void *(*allocate)(size_t alignment, size_t size);
    void (*free)(void *);
};

void *(*llcm_default_allocate(void))(size_t alignment, size_t size) {
    return aligned_alloc;
}

void (*llcm_default_free(void))(void *) { return free; }

struct llcm_allocator llcm_allocator_create_default(void) {
    return (struct llcm_allocator){.allocate = llcm_default_allocate(),
                                   .free = llcm_default_free()};
}