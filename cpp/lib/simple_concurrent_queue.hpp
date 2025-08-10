#pragma once

#include <atomic>
#include <mutex>
#include <stddef.h>
#include <stdint.h>

#include <cassert>
#include <immintrin.h>

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
    alignas(CACHE_LINE_SIZE) class Spinlock {
      public:
        inline void lock() {
            do {
                while (flag_) {
#ifdef __x86_64__
                    _mm_pause();
#endif
                    __asm__ __volatile__("" ::: "memory");
                }
            } while (__atomic_test_and_set(&flag_, __ATOMIC_SEQ_CST));
        }

        inline void unlock() { flag_ = false; }

      private:
        bool flag_ = false;
    } consumer_spin_lock_;

    class SpinlockHandle {
      public:
        SpinlockHandle(Spinlock &spinlock) : spinlock_(spinlock) { spinlock.lock(); }
        ~SpinlockHandle() { spinlock_.unlock(); }

      private:
        Spinlock &spinlock_;
    };
};

template <typename T>
inline void DynamicConcurrentQueue<T>::Push(DynamicConcurrentQueueEntry<T> *value) {
    SpinlockHandle spinlock_handle(consumer_spin_lock_);
    if (head_ == nullptr) {
        head_ = value;
    } else {
        tail_->next_ = value;
    }
    tail_ = value;
}

template <typename T> inline DynamicConcurrentQueueEntry<T> *DynamicConcurrentQueue<T>::TryPop() {
    SpinlockHandle spinlock_handle(consumer_spin_lock_);
    if (head_ == nullptr) {
        return nullptr;
    } else {
        auto *current = head_;
        head_ = head_->next_;
        return current;
    }
}
