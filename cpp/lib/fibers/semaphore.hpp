#pragma once

#include "i_context.hpp"
#include "lib/scmp_dynamic_concurrent_queue.hpp"
#include <cstddef>
#include <cstdint>

template <typename Traits> class Semaphore {
  public:
    void Wait(Traits::FiberT *fiber) {
        auto const local_num_waiters = __atomic_add_fetch(&num_waiters_, 1, __ATOMIC_SEQ_CST);
        if (local_num_waiters > num_signals_) {
            struct SwitchBackTaskEnqueue : public SwitchBackTask {
                SwitchBackTaskEnqueue(Semaphore *semaphore, Traits::FiberT *fiber)
                    : semaphore_(semaphore), fiber_(fiber) {}
                void operator()() override { semaphore_->queue_.Push(&fiber_->queue_entry_); }
                Semaphore *semaphore_ = nullptr;
                Traits::FiberT *fiber_ = nullptr;
            } task(this, fiber);
            fiber->SwitchBack(&task);
        }
    }

    void Signal() {
        auto const local_num_signals = __atomic_add_fetch(&num_signals_, 1, __ATOMIC_SEQ_CST);
        if (local_num_signals > num_waiters_) {
            DynamicConcurrentQueueEntry<typename Traits::FiberT *> *next_queue_entry = nullptr;
            do {
                next_queue_entry = queue_.TryPop();
            } while (next_queue_entry == nullptr);
            auto *fiber = next_queue_entry->element_;
            fiber->Schedule();
        }
    }

  private:
    alignas(Traits::CACHE_LINE_SIZE) uint64_t num_waiters_ = 0;
    alignas(Traits::CACHE_LINE_SIZE) uint64_t num_signals_ = 0;
    DynamicConcurrentQueue<typename Traits::FiberT *> queue_;
};