#pragma once

#include "semaphore.hpp"
#include <cstddef>

template <typename Traits, typename T> class UnbufferedChannel {
  public:
    void Send(Traits::FiberT *fiber, T value) {
        sender_.Wait(fiber);
        value_ = std::move(value);
        if (receiver_.GetCounter() < 0) {
            receiver_.Signal();
        } else {
            waiting_sender_ = fiber;
            fiber->SwitchBack([&]() { receiver_.Signal(); });
        }
    }

    T Receive(Traits::FiberT *fiber) {
        receiver_.Wait();
        T value = std::move(value_);
        if (waiting_sender_ != nullptr) {
            waiting_sender_->Schedule();
            waiting_sender_ = nullptr;
        }
        sender_.Signal();
        return value;
    }

  private:
    T value_;
    Traits::FiberT *waiting_sender_ = nullptr;
    Semaphore<Traits> sender_{1};
    Semaphore<Traits> receiver_{0};
};