#pragma once

#include "context.hpp"
#include "scheduler.hpp"
#include "scmp_dynamic_concurrent_queue.hpp"

class Mutex;

class Coroutine {
  public:
    Coroutine(Scheduler<Coroutine> *scheduler, Context<Scheduler<Coroutine>, Coroutine> *context)
        : scheduler_(scheduler), context_(context) {}

    void Yeild() {
        scheduler_->Schedule(context_);
        context_->SwitchBack();
    }

    void Schedule(ICallable *callable) { scheduler_->Schedule(callable); }

  private:
    friend Mutex;
    void SwitchBack() { context_->SwitchBack(); }
    void Schedule() { scheduler_->Schedule(context_); }

    Scheduler<Coroutine> *scheduler_ = nullptr;
    Context<Scheduler<Coroutine>, Coroutine> *context_ = nullptr;
    DynamicConcurrentQueueEntry<Coroutine *> queue_entry_{.element_ = this};
};