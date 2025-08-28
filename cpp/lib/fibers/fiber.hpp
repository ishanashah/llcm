#pragma once

#include "lib/scmp_dynamic_concurrent_queue.hpp"
#include <utility>

template <typename Traits> class Mutex;
template <typename Traits> class ConditionVariable;
template <typename Traits> class Semaphore;
template <typename Traits, typename T> class UnbufferedChannel;

template <typename Traits> class Fiber {
  public:
    Fiber(Traits::SchedulerT *scheduler, Traits::ContextT *context)
        : scheduler_(scheduler), context_(context) {}

    void Yeild() {
        context_->SwitchBack([&]() { Schedule(); });
    }

    template <typename F> void Schedule(F &&callable) {
        scheduler_->Schedule(std::forward<F>(callable));
    }

    // used for synchronization primitives
    template <typename F> void SwitchBack(F &&task) { context_->SwitchBack(std::forward<F>(task)); }
    void Schedule() { scheduler_->ScheduleContext(context_); }
    DynamicConcurrentQueueEntry<Fiber *> *GetDynamicQueueEntry() { return &queue_entry_; }

  private:
    Traits::SchedulerT *scheduler_ = nullptr;
    Traits::ContextT *context_ = nullptr;
    DynamicConcurrentQueueEntry<Fiber *> queue_entry_{.element_ = this};
};