#pragma once

#include "context.hpp"
#include "fcontext.hpp"
#include "fiber.hpp"
#include "scheduler.hpp"

struct Traits {
    using ContextT = FContext<Traits>;
    using FiberT = Fiber<Traits>;
    using SchedulerT = Scheduler<Traits>;

    static constexpr size_t CACHE_LINE_SIZE = 64;
};