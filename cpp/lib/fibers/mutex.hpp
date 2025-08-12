#pragma once

#include "i_context.hpp"
#include "lib/scmp_dynamic_concurrent_queue.hpp"
#include <cstddef>
#include <cstdint>

template <typename Traits> class Mutex {
  public:
    void Lock(Traits::FiberT *fiber) {
        auto const local_counter = __atomic_fetch_add(&counter_, 1, __ATOMIC_ACQUIRE);
        if (local_counter == 0) {
            return;
        }
        struct SwitchBackTaskEnqueue : public SwitchBackTask {
            SwitchBackTaskEnqueue(Mutex *mutex, Traits::FiberT *fiber)
                : mutex_(mutex), fiber_(fiber) {}
            void operator()() override { mutex_->queue_.Push(&fiber_->queue_entry_); }
            Mutex *mutex_ = nullptr;
            Traits::FiberT *fiber_ = nullptr;
        } task(this, fiber);
        fiber->SwitchBack(&task);
    }

    void Unlock() {
        auto const local_counter = __atomic_sub_fetch(&counter_, 1, __ATOMIC_RELEASE);
        if (local_counter > 0) {
            DynamicConcurrentQueueEntry<typename Traits::FiberT *> *next_queue_entry = nullptr;
            do {
                next_queue_entry = queue_.TryPop();
            } while (next_queue_entry == nullptr);
            auto *fiber = next_queue_entry->element_;
            fiber->Schedule();
        }
    }

  private:
    uint64_t counter_ = 0;
    DynamicConcurrentQueue<typename Traits::FiberT *> queue_;
};