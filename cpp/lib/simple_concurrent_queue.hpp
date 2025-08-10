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
        do {
            while (flag_) {
#ifdef __x86_64__
                _mm_pause();
#endif
                __asm__ __volatile__("" ::: "memory");
            }
        } while (__atomic_test_and_set(&flag_, __ATOMIC_SEQ_CST));
    }

    void unlock() { flag_ = false; }

  private:
    bool flag_ = false;
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
    alignas(CACHE_LINE_SIZE) DynamicConcurrentQueueEntry<T> *head_ = nullptr;
    alignas(CACHE_LINE_SIZE) DynamicConcurrentQueueEntry<T> *tail_ = nullptr;
    alignas(CACHE_LINE_SIZE) Spinlock consumer_spin_lock_;
};

template <typename T>
inline void DynamicConcurrentQueue<T>::Push(DynamicConcurrentQueueEntry<T> *value) {
    consumer_spin_lock_.lock();
    if (head_ == nullptr) {
        head_ = value;
    } else {
        tail_->next_ = value;
    }
    tail_ = value;
    consumer_spin_lock_.unlock();
}

template <typename T> inline DynamicConcurrentQueueEntry<T> *DynamicConcurrentQueue<T>::TryPop() {
    consumer_spin_lock_.lock();
    if (head_ == nullptr) {
        consumer_spin_lock_.unlock();
        return nullptr;
    } else {
        auto *current = head_;
        head_ = head_->next_;
        consumer_spin_lock_.unlock();
        return current;
    }
}
