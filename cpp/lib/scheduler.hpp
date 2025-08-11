#pragma once

#include "callable.hpp"
#include "concurrent_queue.hpp"
#include "context.hpp"
#include <memory>
#include <stddef.h>
#include <stdint.h>

class Scheduler {
  public:
    Scheduler(size_t capacity, size_t stack_size) : queue_(capacity), stack_size_(stack_size) {}

    std::unique_ptr<Context> CreateCoroutine(ICallable *callable) {
        auto *stack = new uint8_t[stack_size_];
        return std::make_unique<Context>(stack, stack_size_, callable);
    }

    void Schedule(std::unique_ptr<Context> context) { queue_.Push(std::move(context)); }

    void Poll() {
        std::optional<std::unique_ptr<Context>> maybe_next = queue_.TryPop();
        if (!maybe_next.has_value()) {
            return;
        }
        std::unique_ptr<Context> next = std::move(maybe_next.value());
        main_context_.Swap(*next);
    }

  private:
    ConcurrentQueue<std::unique_ptr<Context>> queue_;
    Context main_context_;
    size_t stack_size_;
};