#pragma once

#include "callable.hpp"
#include "concurrent_queue.hpp"
#include "context.hpp"
#include <memory>
#include <stddef.h>
#include <stdint.h>

template <typename Coroutine> class Scheduler {
  public:
    Scheduler(size_t capacity, size_t stack_size) : queue_(capacity), stack_size_(stack_size) {}

    void Schedule(ICallable *callable) {
        auto context = std::make_unique<Context<Scheduler, Coroutine>>(this, stack_size_, callable);
        Schedule(std::move(context));
    }

    void Poll() {
        std::optional<std::unique_ptr<Context<Scheduler, Coroutine>>> maybe_next = queue_.TryPop();
        if (!maybe_next.has_value()) {
            return;
        }
        std::unique_ptr<Context<Scheduler, Coroutine>> next = std::move(maybe_next.value());
        next->Switch();
        if (next->IsActive()) {
            Schedule(std::move(next));
        }
    }

  private:
    void Schedule(std::unique_ptr<Context<Scheduler, Coroutine>> context) {
        queue_.Push(std::move(context));
    }

  private:
    ConcurrentQueue<std::unique_ptr<Context<Scheduler, Coroutine>>> queue_;
    size_t stack_size_;
};