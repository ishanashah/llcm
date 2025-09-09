#pragma once

#include <atomic>
#include <optional>
#include <stddef.h>
#include <stdint.h>
#include <vector>

template <typename T> class SpscConcurrentQueue {
  public:
    SpscConcurrentQueue(size_t capacity);

    size_t GetCapacity() const { return mask_ + 1; }

    template <typename U> bool Push(U &&);
    std::optional<T> TryPop();

  private:
    static constexpr size_t CACHE_LINE_SIZE = 64;
    struct Entry {
        alignas(CACHE_LINE_SIZE) T element_;
    };
    static_assert(alignof(Entry) >= CACHE_LINE_SIZE, "");
    static constexpr uint64_t RoundUpPow2(uint64_t x) {
        return x == 1 ? 1 : 1 << (64 - __builtin_clzl(x - 1));
    }

  private:
    std::vector<Entry> array_;
    size_t mask_ = 0;

    alignas(CACHE_LINE_SIZE) std::atomic<uint64_t> read_counter_ = 0;
    alignas(CACHE_LINE_SIZE) std::atomic<uint64_t> write_counter_ = 0;
};

template <typename T> SpscConcurrentQueue<T>::SpscConcurrentQueue(size_t capacity) {
    capacity = RoundUpPow2(capacity);
    array_ = std::vector<Entry>(capacity);
    mask_ = capacity - 1;
}

template <typename T> template <typename U> bool SpscConcurrentQueue<T>::Push(U &&value) {
    auto const local_read_counter = read_counter_.load(std::memory_order_acquire);
    auto const local_write_counter = write_counter_.load(std::memory_order_relaxed);
    auto const size = local_write_counter - local_read_counter;
    if (size >= mask_ + 1) {
        return false;
    }

    struct Entry *entry = &array_[local_write_counter & mask_];
    entry->element_ = std::forward<U>(value);
    write_counter_.store(local_write_counter + 1, std::memory_order_release);
    return true;
}

template <typename T> std::optional<T> SpscConcurrentQueue<T>::TryPop() {
    auto const local_write_counter = write_counter_.load(std::memory_order_acquire);
    auto const local_read_counter = read_counter_.load(std::memory_order_relaxed);
    auto const size = local_write_counter - local_read_counter;
    if (size == 0) {
        return {};
    }

    struct Entry *entry = &array_[local_read_counter & mask_];
    T result = std::move(entry->element_);
    read_counter_.store(local_read_counter + 1, std::memory_order_release);
    return result;
}
