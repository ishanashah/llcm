#pragma once

#include "coroutine.hpp"
#include "scmp_dynamic_concurrent_queue.hpp"
#include <cstddef>
#include <cstdint>

class Mutex {
  public:
    void Lock(Coroutine *coroutine) {
        auto const local_counter = __atomic_fetch_add(&counter_, 1, __ATOMIC_ACQUIRE);
        if (local_counter == 0) {
            return;
        }
        struct SwitchBackTaskEnqueue : public SwitchBackTask {
            SwitchBackTaskEnqueue(Mutex *mutex, Coroutine *coroutine)
                : mutex_(mutex), coroutine_(coroutine) {}
            void operator()() override { mutex_->queue_.Push(&coroutine_->queue_entry_); }
            Mutex *mutex_ = nullptr;
            Coroutine *coroutine_ = nullptr;
        } task(this, coroutine);
        coroutine->SwitchBack(&task);
    }

    void Unlock() {
        auto const local_counter = __atomic_sub_fetch(&counter_, 1, __ATOMIC_RELEASE);
        if (local_counter > 0) {
            DynamicConcurrentQueueEntry<Coroutine *> *next_queue_entry = nullptr;
            do {
                next_queue_entry = queue_.TryPop();
            } while (next_queue_entry == nullptr);
            auto *coroutine = next_queue_entry->element_;
            coroutine->Schedule();
        }
    }

  private:
    uint64_t counter_ = 0;
    DynamicConcurrentQueue<Coroutine *> queue_;
};