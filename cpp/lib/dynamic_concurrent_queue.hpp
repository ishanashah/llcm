#pragma once

#include <stddef.h>
#include <stdint.h>

static constexpr size_t DYNAMIC_CONCURRENT_QUEUE_CACHE_LINE_SIZE = 64;

struct DynamicConcurrentQueueEntry {
    // alignas(DYNAMIC_CONCURRENT_QUEUE_CACHE_LINE_SIZE) volatile uint64_t aba_counter_ = 0;
    DynamicConcurrentQueueEntry *next_ = nullptr;
    bool push_in_progress_ = false;
};

class DynamicConcurrentQueue {
  public:
    void Push(DynamicConcurrentQueueEntry *);
    DynamicConcurrentQueueEntry *Pop();

  private:
    static DynamicConcurrentQueueEntry EMPTY_SENTINEL;

  private:
    DynamicConcurrentQueueEntry *head_ = &EMPTY_SENTINEL;
    DynamicConcurrentQueueEntry *tail_ = nullptr;
};

inline void DynamicConcurrentQueue::Push(DynamicConcurrentQueueEntry *value) {
    value->next_ = &EMPTY_SENTINEL;
    value->push_in_progress_ = true;
    DynamicConcurrentQueueEntry *old_tail = __atomic_exchange_n(&tail_, value, __ATOMIC_SEQ_CST);
    old_tail->next_ = value;
    while (old_tail->push_in_progress_) {
    }
    value->push_in_progress_ = false;
}

inline DynamicConcurrentQueueEntry *DynamicConcurrentQueue::Pop() {
    DynamicConcurrentQueueEntry *current_head = nullptr;
    while (current_head == nullptr) {
        while (head_ == nullptr) {
        }
        current_head = __atomic_exchange_n(&head_, nullptr, __ATOMIC_SEQ_CST);
    }
    head_ = current_head->next_ == nullptr ? &EMPTY_SENTINEL : current_head->next_;
    if (current_head == &EMPTY_SENTINEL) {
        return nullptr;
    } else {
        current_head->next_ = nullptr;
        return current_head;
    }
}
