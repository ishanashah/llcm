#pragma once

#include <atomic>
#include <iostream>
#include <stddef.h>
#include <stdint.h>

template <typename T> struct DynamicConcurrentQueueEntry {
    DynamicConcurrentQueueEntry<T> *next_ = nullptr;
    T element_{};
};

template <typename T> class DynamicConcurrentQueue {
  public:
    void Push(DynamicConcurrentQueueEntry<T> *);
    DynamicConcurrentQueueEntry<T> *TryPop();

  private:
    static constexpr size_t CACHE_LINE_SIZE = 64;
    alignas(CACHE_LINE_SIZE) DynamicConcurrentQueueEntry<T> EMPTY_SENTINEL{
        .next_ = &EMPTY_SENTINEL};
    alignas(CACHE_LINE_SIZE) std::atomic<DynamicConcurrentQueueEntry<T> *> head_ = &EMPTY_SENTINEL;
    alignas(CACHE_LINE_SIZE) std::atomic<DynamicConcurrentQueueEntry<T> *> tail_ = &EMPTY_SENTINEL;
};

template <typename T>
inline void DynamicConcurrentQueue<T>::Push(DynamicConcurrentQueueEntry<T> *value) {
    value->next_ = &EMPTY_SENTINEL;
    auto *const old_tail = tail_.exchange(value);
    if (old_tail == &EMPTY_SENTINEL) {
        auto *expected_empty = &EMPTY_SENTINEL;
        while (!head_.compare_exchange_strong(expected_empty, value)) {
        }
    } else {
        old_tail->next_ = value;
    }
}

template <typename T> inline DynamicConcurrentQueueEntry<T> *DynamicConcurrentQueue<T>::TryPop() {
    DynamicConcurrentQueueEntry<T> *local_head = nullptr;
    while (local_head == nullptr) {
        local_head = head_.exchange(nullptr);
    }
    if (local_head == &EMPTY_SENTINEL) {
        head_ = &EMPTY_SENTINEL;
        return nullptr;
    }

    auto *current = local_head;
    local_head = local_head->next_;
    head_ = local_head;
    if (local_head == &EMPTY_SENTINEL) {
        auto *expected_tail = current;
        if (!tail_.compare_exchange_strong(expected_tail, &EMPTY_SENTINEL)) {
            while (current->next_ == &EMPTY_SENTINEL) {
            }
            head_ = current->next_;
        }
    }
    current->next_ = nullptr;
    return current;
}
