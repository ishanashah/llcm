#pragma once

#include <stddef.h>
#include <stdint.h>

/* public */

uint64_t llcm_round_up_pow2(uint64_t x) {
    return x == 1 ? 1 : 1 << (64 - __builtin_clzl(x - 1));
}