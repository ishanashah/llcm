#pragma once

#include <atomic>
#include <mutex>
#include <stddef.h>
#include <stdint.h>

#include <cassert>
#include <immintrin.h>
#include <iostream>

class Spinlock {
  public:
    void lock() {
        while (flag.test_and_set(std::memory_order_acquire)) {
            do {
#ifdef __x86_64__
                _mm_pause();
#endif
            } while (flag.test(std::memory_order_relaxed));
        }
    }

    void unlock() { flag.clear(std::memory_order_release); }

  private:
    std::atomic_flag flag = ATOMIC_FLAG_INIT;
};

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
    alignas(CACHE_LINE_SIZE) std::atomic<DynamicConcurrentQueueEntry<T> *> head_ = nullptr;
    alignas(CACHE_LINE_SIZE) std::atomic<DynamicConcurrentQueueEntry<T> *> tail_ = nullptr;
    alignas(CACHE_LINE_SIZE) Spinlock consumer_spin_lock_;
};

template <typename T>
inline void DynamicConcurrentQueue<T>::Push(DynamicConcurrentQueueEntry<T> *value) {
    value->next_ = nullptr;
    auto *const old_tail = tail_.exchange(value);
    if (old_tail == nullptr) {
        head_ = value;
    } else {
        old_tail->next_ = value;
    }
}

template <typename T> inline DynamicConcurrentQueueEntry<T> *DynamicConcurrentQueue<T>::TryPop() {
    consumer_spin_lock_.lock();
    if (head_ == nullptr) {
        consumer_spin_lock_.unlock();
        return nullptr;
    }
    DynamicConcurrentQueueEntry<T> *current = head_;
    head_ = current->next_;
    if (head_ == nullptr) {
        auto *expected_tail = current;
        if (!tail_.compare_exchange_strong(expected_tail, nullptr)) {
            while (current->next_ == nullptr) {
                __asm__ __volatile__("" ::: "memory");
            }
            head_ = current->next_;
        }
    }
    consumer_spin_lock_.unlock();
    current->next_ = nullptr;
    return current;
}
