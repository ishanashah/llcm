#pragma once

#include "i_context.hpp"
#include "lib/scmp_dynamic_concurrent_queue.hpp"
#include "mutex.hpp"
#include <algorithm>
#include <cstddef>
#include <cstdint>

template <typename Traits, typename T> class UnbufferedChannel {
  public:
    void Send(Traits::FiberT *fiber, T value) {
        auto const local_num_senders = __atomic_fetch_add(&num_senders_, 1, __ATOMIC_SEQ_CST);
        auto const local_num_receivers = num_receivers_;
        if (local_num_senders > local_num_receivers) {
            struct SwitchBackTaskEnqueue : public SwitchBackTask {
                SwitchBackTaskEnqueue(UnbufferedChannel *channel, Traits::FiberT *fiber)
                    : channel_(channel), fiber_(fiber) {}
                void operator()() override { channel_->queue_.Push(&fiber_->queue_entry_); }
                UnbufferedChannel *channel_ = nullptr;
                Traits::FiberT *fiber_ = nullptr;
            } task(this, fiber);
            fiber->SwitchBack(&task);
        } else {
            ScheduleFiber(&receivers_);
        }

        while (__atomic_load_n(&writer_idx_, __ATOMIC_SEQ_CST) != local_num_senders) {
        }
        value_ = std::move(value);
        __atomic_store_n(&reader_idx_, local_num_senders + 1, __ATOMIC_SEQ_CST);
    }

    T Receive(Traits::FiberT *fiber) {
        auto const local_num_receivers = __atomic_fetch_add(&num_receivers_, 1, __ATOMIC_SEQ_CST);
        auto const local_num_senders = num_receivers_;
        if (local_num_receivers > local_num_senders) {
            struct SwitchBackTaskEnqueue : public SwitchBackTask {
                SwitchBackTaskEnqueue(UnbufferedChannel *channel, Traits::FiberT *fiber)
                    : channel_(channel), fiber_(fiber) {}
                void operator()() override { channel_->queue_.Push(&fiber_->queue_entry_); }
                UnbufferedChannel *channel_ = nullptr;
                Traits::FiberT *fiber_ = nullptr;
            } task(this, fiber);
            fiber->SwitchBack(&task);
        } else {
            ScheduleFiber(&senders_);
        }

        T value;
        while (__atomic_load_n(&reader_idx_, __ATOMIC_SEQ_CST) != local_num_receivers + 1) {
        }
        value = std::move(value_);
        __atomic_store_n(&writer_idx_, local_num_receivers + 1, __ATOMIC_SEQ_CST);
        return value;
    }

  private:
    static void ScheduleFiber(DynamicConcurrentQueue<typename Traits::FiberT *> *queue) {
        DynamicConcurrentQueueEntry<typename Traits::FiberT *> *next_queue_entry = nullptr;
        do {
            next_queue_entry = queue->TryPop();
        } while (next_queue_entry == nullptr);
        auto *fiber = next_queue_entry->element_;
        fiber->Schedule();
    }

    alignas(Traits::CACHE_LINE_SIZE) uint64_t num_senders_ = 0;
    alignas(Traits::CACHE_LINE_SIZE) uint64_t num_receivers_ = 0;
    DynamicConcurrentQueue<typename Traits::FiberT *> senders_;
    DynamicConcurrentQueue<typename Traits::FiberT *> receivers_;
    alignas(Traits::CACHE_LINE_SIZE) uint64_t writer_idx_ = 0;
    alignas(Traits::CACHE_LINE_SIZE) uint64_t reader_idx_ = 0;
    alignas(Traits::CACHE_LINE_SIZE) T value_;
};