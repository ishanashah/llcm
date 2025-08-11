#pragma once

#include "context.hpp"
#include "coroutine.hpp"
#include "scheduler.hpp"

struct Traits {
    template <typename F> using ContextT = Context<Traits, F>;

    using CoroutineT = Coroutine<Traits>;
    using SchedulerT = Scheduler<Traits>;
};