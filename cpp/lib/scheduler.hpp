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
        auto *context = new Context<Scheduler, Coroutine>(this, stack_size_, callable);
        Schedule(std::move(context));
    }

    bool Poll() {
        std::optional<Context<Scheduler, Coroutine> *> maybe_next = queue_.TryPop();
        if (!maybe_next.has_value()) {
            return false;
        }
        Context<Scheduler, Coroutine> *next = std::move(maybe_next.value());
        SwitchBackTask *task = next->Switch();
        (*task)();
        // if (!next->IsActive()) {
        //     delete next;
        // }
        return true;
    }

  private:
    friend Coroutine;
    void Schedule(Context<Scheduler, Coroutine> *context) { queue_.Push(std::move(context)); }

  private:
    ConcurrentQueue<Context<Scheduler, Coroutine> *> queue_;
    size_t stack_size_;
};