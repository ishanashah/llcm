#pragma once

#include "lib/concurrent_queue.hpp"
#include <memory>
#include <stddef.h>
#include <stdint.h>

template <typename Traits> class Scheduler {
  public:
    Scheduler(size_t capacity, size_t stack_size) : queue_(capacity), stack_size_(stack_size) {}

    template <typename F> void Schedule(F &&callable) {
        auto *context = new Traits::ContextT(this, stack_size_, std::forward<F>(callable));
        ScheduleContext(std::move(context));
    }

    bool Poll() {
        std::optional<typename Traits::ContextT *> maybe_next = queue_.TryPop();
        if (!maybe_next.has_value()) {
            return false;
        }
        typename Traits::ContextT *next = std::move(maybe_next.value());
        next->Switch();
        if (!next->IsActive()) {
            delete next;
        }
        return true;
    }

  private:
    friend Traits::FiberT;
    void ScheduleContext(Traits::ContextT *context) { queue_.Push(std::move(context)); }

  private:
    ConcurrentQueue<typename Traits::ContextT *> queue_;
    size_t stack_size_;
};