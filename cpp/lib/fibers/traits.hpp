#pragma once

#include "context.hpp"
#include "context3.hpp"
#include "fiber.hpp"
#include "scheduler.hpp"

struct Traits {
    template <typename F> using ContextT = Context3<Traits, F>;

    using FiberT = Fiber<Traits>;
    using SchedulerT = Scheduler<Traits>;

    static constexpr size_t CACHE_LINE_SIZE = 64;
};