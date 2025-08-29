#pragma once

#include "lib/concurrent_queue.hpp"
#include <memory>
#include <stddef.h>
#include <stdint.h>

template <typename Traits> class Scheduler {
  public:
    Scheduler(size_t capacity, size_t stack_size)
        : queue_(capacity), stack_factory_(capacity, stack_size), stack_size_(stack_size) {}

    template <typename F> bool TryCreateFiber(F &&callable) {
        if (!queue_.TryReserveSizeBeforePush(1)) {
            return false;
        }
        auto *context =
            new Traits::ContextT(this, stack_factory_.allocate(), std::forward<F>(callable));
        ScheduleContext(std::move(context));
        return true;
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
            queue_.UnreserveSizeAfterPop(1);
        }
        return true;
    }

  private:
    friend Traits::FiberT;
    void ScheduleContext(Traits::ContextT *context) { queue_.Push(std::move(context)); }

  private:
    ConcurrentQueue<typename Traits::ContextT *> queue_;
    Traits::StackFactoryT stack_factory_;
    size_t stack_size_;
};