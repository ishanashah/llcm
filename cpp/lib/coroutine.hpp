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
        struct SwitchBackTaskSchedule : public SwitchBackTask {
            SwitchBackTaskSchedule(Coroutine *coroutine) : coroutine_(coroutine) {}
            void operator()() override { coroutine_->Schedule(); }
            Coroutine *coroutine_ = nullptr;
        } task(this);
        context_->SwitchBack(&task);
    }

    void Schedule(ICallable *callable) { scheduler_->Schedule(callable); }

  private:
    friend Mutex;
    void SwitchBack(SwitchBackTask *task) { context_->SwitchBack(task); }
    void Schedule() { scheduler_->Schedule(context_); }

    Scheduler<Coroutine> *scheduler_ = nullptr;
    Context<Scheduler<Coroutine>, Coroutine> *context_ = nullptr;
    DynamicConcurrentQueueEntry<Coroutine *> queue_entry_{.element_ = this};
};