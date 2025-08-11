#pragma once

#include <cassert>
#include <cstdlib>

[[noreturn]] inline void unreachable() {
    // Uses compiler specific extensions if possible.
    // Even if no extension is used, undefined behavior is still raised by
    // an empty function body and the noreturn attribute.
#if defined(_MSC_VER) && !defined(__clang__)   // MSVC
    __assume(false);
#else   // GCC, Clang
    __builtin_unreachable();
#endif
}

#define DEBUG_ASSERT(condition) assert(condition)

#define PROD_ASSERT(condition)                                                                     \
    do {                                                                                           \
        if (!(condition)) {                                                                        \
            std::abort();                                                                          \
        }                                                                                          \
    } while (0);

#define DIE()                                                                                      \
    do {                                                                                           \
        std::abort();                                                                              \
        unreachable();                                                                             \
    } while (0);
