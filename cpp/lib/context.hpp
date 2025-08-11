#pragma once

#include "callable.hpp"
#include "utils.hpp"
#include <cstddef>
#include <cstdint>
#include <future>
#include <iostream>
#include <ucontext.h>
#include <vector>

struct SwitchBackTask {
    virtual void operator()() = 0;
};

template <typename SCHEDULER, typename COROUTINE> class Context {
  public:
    Context(SCHEDULER *scheduler, size_t stack_size, ICallable *callable)
        : stack_(stack_size), callable_(callable), scheduler_(scheduler) {
        int ret = getcontext(&context_);
        PROD_ASSERT(ret == 0)
        context_.uc_stack.ss_sp = &stack_[0];
        context_.uc_stack.ss_size = stack_size;
        makecontext(&context_, (void (*)()) Invoke, 1, this);
    }

    bool IsActive() const { return callable_ != nullptr; }

    void Switch() {
        ucontext_t main_context;
        main_context_ = &main_context;
        int ret = swapcontext(main_context_, &context_);
        PROD_ASSERT(ret == 0);
        (*switch_back_task_)();
        switch_back_task_ = nullptr;
    }

    void SwitchBack(SwitchBackTask *task) {
        auto const *local_main_context = main_context_;
        main_context_ = nullptr;
        switch_back_task_ = task;
        int ret = swapcontext(&context_, local_main_context);
        PROD_ASSERT(ret == 0);
    }

  private:
    static void Invoke(Context *context) {
        COROUTINE coroutine(context->scheduler_, context);
        (*context->callable_)(&coroutine);
        context->callable_ = nullptr;
        setcontext(context->main_context_);
        DIE();   // unreachable
    }

  private:
    ucontext_t context_{};
    ucontext_t *main_context_ = nullptr;
    std::vector<uint8_t> stack_;
    ICallable *callable_ = nullptr;
    SCHEDULER *scheduler_ = nullptr;
    SwitchBackTask *switch_back_task_ = nullptr;
};