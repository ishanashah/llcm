#pragma once

#include "i_context.hpp"
#include "lib/scmp_dynamic_concurrent_queue.hpp"
#include "mutex.hpp"
#include <cstddef>
#include <cstdint>

template <typename Traits> class ConditionVariable {
  public:
    void Wait(Traits::FiberT *fiber, Mutex<Traits> *mutex) {
        __atomic_fetch_add(&counter_, 1, __ATOMIC_ACQUIRE);
        mutex->Unlock();
        struct SwitchBackTaskEnqueue : public SwitchBackTask {
            SwitchBackTaskEnqueue(ConditionVariable *condition_variable, Traits::FiberT *fiber)
                : condition_variable_(condition_variable), fiber_(fiber) {}
            void operator()() override { condition_variable_->queue_.Push(&fiber_->queue_entry_); }
            ConditionVariable *condition_variable_ = nullptr;
            Traits::FiberT *fiber_ = nullptr;
        } task(this, fiber);
        fiber->SwitchBack(&task);
        mutex->Lock(fiber);
    }

    void Signal() {
        auto local_counter = counter_;
        while (local_counter > 0) {
            if (__atomic_compare_exchange_n(&counter_, &local_counter, local_counter - 1, true,
                                            __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST)) {
                ScheduleFiber();
                return;
            }
        }
    }

    void Broadcast() {
        auto const local_counter = __atomic_exchange_n(&counter_, 0, __ATOMIC_SEQ_CST);
        for (auto i = 0; i < local_counter; i++) {
            ScheduleFiber();
        }
    }

  private:
    void ScheduleFiber() {
        DynamicConcurrentQueueEntry<typename Traits::FiberT *> *next_queue_entry = nullptr;
        do {
            next_queue_entry = queue_.TryPop();
        } while (next_queue_entry == nullptr);
        auto *fiber = next_queue_entry->element_;
        fiber->Schedule();
    }

    uint64_t counter_ = 0;
    DynamicConcurrentQueue<typename Traits::FiberT *> queue_;
};