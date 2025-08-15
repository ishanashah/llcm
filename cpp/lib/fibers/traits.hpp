#pragma once

#include "context.hpp"
#include "fcontext.hpp"
#include "fiber.hpp"
#include "scheduler.hpp"

struct Traits {
    template <typename F> using ContextT = FContext<Traits, F>;

    using FiberT = Fiber<Traits>;
    using SchedulerT = Scheduler<Traits>;

    static constexpr size_t CACHE_LINE_SIZE = 64;
};