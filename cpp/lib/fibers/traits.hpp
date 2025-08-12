#pragma once

#include "context.hpp"
#include "fiber.hpp"
#include "scheduler.hpp"

struct Traits {
    template <typename F> using ContextT = Context<Traits, F>;

    using FiberT = Fiber<Traits>;
    using SchedulerT = Scheduler<Traits>;
};