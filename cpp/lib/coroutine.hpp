#pragma once

#include "i_context.hpp"
#include "scmp_dynamic_concurrent_queue.hpp"
#include <utility>

template <typename Traits> class Mutex;

template <typename Traits> class Coroutine {
  public:
    Coroutine(Traits::SchedulerT *scheduler, IContext *context)
        : scheduler_(scheduler), context_(context) {}

    void Yeild() {
        struct SwitchBackTaskSchedule : public SwitchBackTask {
            SwitchBackTaskSchedule(Coroutine *coroutine) : coroutine_(coroutine) {}
            void operator()() override { coroutine_->Schedule(); }
            Coroutine *coroutine_ = nullptr;
        } task(this);
        context_->SwitchBack(&task);
    }

    template <typename F> void Schedule(F &&callable) {
        scheduler_->Schedule(std::forward<F>(callable));
    }

  private:
    friend Mutex<Traits>;
    void SwitchBack(SwitchBackTask *task) { context_->SwitchBack(task); }
    void Schedule() { scheduler_->ScheduleContext(context_); }

    Traits::SchedulerT *scheduler_ = nullptr;
    IContext *context_ = nullptr;
    DynamicConcurrentQueueEntry<Coroutine *> queue_entry_{.element_ = this};
};