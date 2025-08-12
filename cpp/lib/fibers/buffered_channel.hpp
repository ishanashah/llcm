#pragma once

#include "lib/concurrent_queue.hpp"
#include "semaphore.hpp"
#include <cstddef>

template <typename Traits, typename T> class BufferedChannel {
  public:
    BufferedChannel(size_t capacity) : sender_(capacity), queue_(capacity) {}

    template <typename U> void Send(Traits::FiberT *fiber, U &&value) {
        sender_.Wait(fiber);
        queue_.push(std::forward<U>(value));
        receiver_.Signal();
    }

    T Receive(Traits::FiberT *fiber) {
        receiver_.Wait(fiber);
        T value = queue_.ForcePop();
        sender_.Signal();
        return value;
    }

  private:
    Semaphore<Traits> sender_;
    Semaphore<Traits> receiver_;
    ConcurrentQueue<T> queue_;
};