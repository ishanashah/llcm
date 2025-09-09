#pragma once

#include <optional>
#include <stddef.h>
#include <stdint.h>
#include <vector>

template <typename T> class SpscConcurrentQueue {
  public:
    SpscConcurrentQueue(size_t capacity);

    size_t GetCapacity() const { return mask_ + 1; }

    bool TryReserveSizeBeforePush(size_t num_new_entries);
    void UnreserveSizeAfterPop(size_t num_old_entries);

    template <typename U> void Push(U &&);
    std::optional<T> TryPop();
    T ForcePop();

  private:
    static constexpr size_t CACHE_LINE_SIZE = 64;
    struct Entry {
        alignas(CACHE_LINE_SIZE) volatile uint64_t aba_counter_ = 0;
        alignas(CACHE_LINE_SIZE) T element_;
    };
    static_assert(alignof(Entry) >= CACHE_LINE_SIZE, "");
    static constexpr uint64_t RoundUpPow2(uint64_t x) {
        return x == 1 ? 1 : 1 << (64 - __builtin_clzl(x - 1));
    }

  private:
    std::vector<Entry> array_;
    size_t mask_ = 0;

    alignas(CACHE_LINE_SIZE) uint64_t read_counter_ = 0;
    alignas(CACHE_LINE_SIZE) uint64_t write_counter_ = 0;
    alignas(CACHE_LINE_SIZE) uint64_t reserved_push_size_ = 0;
};

template <typename T> SpscConcurrentQueue<T>::SpscConcurrentQueue(size_t capacity) {
    capacity = RoundUpPow2(capacity);
    if (capacity < 2) {
        capacity = 2;   // capacity must be at least 2 for aba_counter
    }
    array_ = std::vector<Entry>(capacity);
    for (uint64_t i = 0; i < capacity; i++) {
        array_[i].aba_counter_ = i;
    }
    mask_ = capacity - 1;
}

template <typename T>
bool SpscConcurrentQueue<T>::TryReserveSizeBeforePush(size_t num_new_entries) {
    uint64_t const reserved_push_size =
        __atomic_fetch_add(&reserved_push_size_, num_new_entries, __ATOMIC_SEQ_CST);
    if (reserved_push_size > mask_) {
        __atomic_fetch_sub(&reserved_push_size_, num_new_entries, __ATOMIC_SEQ_CST);
        return false;
    }
    return true;
}

template <typename T> void SpscConcurrentQueue<T>::UnreserveSizeAfterPop(size_t num_old_entries) {
    __atomic_fetch_sub(&reserved_push_size_, num_old_entries, __ATOMIC_SEQ_CST);
}

template <typename T> template <typename U> void SpscConcurrentQueue<T>::Push(U &&value) {
    uint64_t const reserved_write_counter =
        __atomic_fetch_add(&write_counter_, 1, __ATOMIC_SEQ_CST);
    struct Entry *entry = &array_[reserved_write_counter & mask_];
    while (entry->aba_counter_ != reserved_write_counter) {
    }
    entry->element_ = std::forward<U>(value);
    __asm__ __volatile__("" ::: "memory");
    entry->aba_counter_ = reserved_write_counter + 1;
}

template <typename T> std::optional<T> SpscConcurrentQueue<T>::TryPop() {
    uint64_t const local_write_counter = write_counter_;
    uint64_t *read_ptr = &read_counter_;
    uint64_t local_read_counter = *read_ptr;
    while (local_read_counter < local_write_counter) {
        if (__atomic_compare_exchange_n(read_ptr, &local_read_counter, local_read_counter + 1,
                                        false, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST)) {
            struct Entry *entry = &array_[local_read_counter & mask_];
            while (entry->aba_counter_ != local_read_counter + 1) {
            }
            T read_value = std::move(entry->element_);
            __asm__ __volatile__("" ::: "memory");
            entry->aba_counter_ = local_read_counter + mask_ + 1;
            return read_value;
        }
    }
    return std::nullopt;
}

template <typename T> T SpscConcurrentQueue<T>::ForcePop() {
    uint64_t const reserved_read_counter = __atomic_fetch_add(&read_counter_, 1, __ATOMIC_SEQ_CST);
    struct Entry *entry = &array_[reserved_read_counter & mask_];
    while (entry->aba_counter_ != reserved_read_counter + 1) {
    }
    T read_value = std::move(entry->element_);
    __asm__ __volatile__("" ::: "memory");
    entry->aba_counter_ = reserved_read_counter + mask_ + 1;
    return read_value;
}
