#pragma once

#include "callable.hpp"
#include "utils.hpp"
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <ucontext.h>
#include <vector>

template <typename SCHEDULER, typename COROUTINE> class Context {
  public:
    Context(SCHEDULER *scheduler, size_t stack_size, ICallable *callable)
        : stack_(stack_size), callable_(callable), coroutine_(scheduler, this) {
        int ret = getcontext(&context_);
        PROD_ASSERT(ret == 0)
        context_.uc_stack.ss_sp = &stack_[0];
        context_.uc_stack.ss_size = stack_size;
        makecontext(&context_, (void (*)()) Invoke, 1, this);
    }

    bool IsActive() const { return callable_ != nullptr; }

    void Switch() {
        ucontext_t main_context_;
        context_.uc_link = &main_context_;
        int ret = swapcontext(&main_context_, &context_);
        PROD_ASSERT(ret == 0);
    }

    void SwitchBack() {
        int ret = swapcontext(&context_, context_.uc_link);
        PROD_ASSERT(ret == 0);
    }

  private:
    void operator()() {
        (*callable_)(&coroutine_);
        callable_ = nullptr;
        SwitchBack();
    }

    static void Invoke(Context *context) { (*context)(); }

  private:
    ucontext_t context_;
    std::vector<uint8_t> stack_;
    ICallable *callable_ = nullptr;
    COROUTINE coroutine_;
};