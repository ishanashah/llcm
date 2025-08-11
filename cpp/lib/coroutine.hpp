#pragma once

#include "context.hpp"
#include "scheduler.hpp"

class Coroutine {
  public:
    Coroutine(Scheduler<Coroutine> *scheduler, Context<Scheduler<Coroutine>, Coroutine> *context)
        : scheduler_(scheduler), context_(context) {}

    void Yeild() { context_->SwitchBack(); }

    void Schedule(ICallable *callable) { scheduler_->Schedule(callable); }

  private:
    Scheduler<Coroutine> *scheduler_ = nullptr;
    Context<Scheduler<Coroutine>, Coroutine> *context_ = nullptr;
};