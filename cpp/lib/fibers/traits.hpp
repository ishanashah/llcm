#pragma once

#include "fcontext.hpp"
#include "fiber.hpp"
#include "scheduler.hpp"
#include "stack_factory_pool.hpp"

struct Traits {
    using ContextT = FContext<Traits>;
    using FiberT = Fiber<Traits>;
    using SchedulerT = Scheduler<Traits>;
    using StackFactoryT = StackFactoryPool<Traits>;
    using StackT = StackFactoryPool<Traits>::Stack;

    static constexpr size_t CACHE_LINE_SIZE = 64;
};