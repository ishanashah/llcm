#pragma once

#include "lib/scmp_dynamic_concurrent_queue.hpp"
#include <cstddef>
#include <cstdint>

template <typename Traits> class Semaphore {
  public:
    Semaphore() : Semaphore(0) {}
    Semaphore(uint64_t count) : counter_(count) {}

    int64_t GetCounter() const { return __atomic_load_n(&counter_, __ATOMIC_SEQ_CST); }

    void Wait(Traits::FiberT *fiber) {
        auto const local_counter = __atomic_sub_fetch(&counter_, 1, __ATOMIC_SEQ_CST);
        if (local_counter < 0) {
            fiber->SwitchBack([&]() { queue_.Push(fiber->GetDynamicQueueEntry()); });
        }
    }

    void Signal() {
        auto const local_counter = __atomic_fetch_add(&counter_, 1, __ATOMIC_SEQ_CST);
        if (local_counter < 0) {
            DynamicConcurrentQueueEntry<typename Traits::FiberT *> *next_queue_entry = nullptr;
            do {
                next_queue_entry = queue_.TryPop();
            } while (next_queue_entry == nullptr);
            auto *fiber = next_queue_entry->element_;
            fiber->Schedule();
        }
    }

  private:
    alignas(Traits::CACHE_LINE_SIZE) int64_t counter_ = 0;
    DynamicConcurrentQueue<typename Traits::FiberT *> queue_;
};