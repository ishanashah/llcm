#pragma once

#include "i_context.hpp"
#include "lib/scmp_dynamic_concurrent_queue.hpp"
#include <utility>

template <typename Traits> class Mutex;
template <typename Traits> class ConditionVariable;

template <typename Traits> class Fiber {
  public:
    Fiber(Traits::SchedulerT *scheduler, IContext *context)
        : scheduler_(scheduler), context_(context) {}

    void Yeild() {
        struct SwitchBackTaskSchedule : public SwitchBackTask {
            SwitchBackTaskSchedule(Fiber *fiber) : fiber_(fiber) {}
            void operator()() override { fiber_->Schedule(); }
            Fiber *fiber_ = nullptr;
        } task(this);
        context_->SwitchBack(&task);
    }

    template <typename F> void Schedule(F &&callable) {
        scheduler_->Schedule(std::forward<F>(callable));
    }

  private:
    friend Mutex<Traits>;
    friend ConditionVariable<Traits>;
    void SwitchBack(SwitchBackTask *task) { context_->SwitchBack(task); }
    void Schedule() { scheduler_->ScheduleContext(context_); }

    Traits::SchedulerT *scheduler_ = nullptr;
    IContext *context_ = nullptr;
    DynamicConcurrentQueueEntry<Fiber *> queue_entry_{.element_ = this};
};