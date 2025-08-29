#pragma once

#include "fcontext.hpp"
#include "fiber.hpp"
#include "scheduler.hpp"
#include "stack_factory.hpp"

struct Traits {
    using ContextT = FContext<Traits>;
    using FiberT = Fiber<Traits>;
    using SchedulerT = Scheduler<Traits>;
    using StackFactoryT = StackFactoryBasic<Traits>;
    using StackT = StackFactoryBasic<Traits>::Stack;

    static constexpr size_t CACHE_LINE_SIZE = 64;
};