#pragma once

#include "i_context.hpp"
#include "lib/utils.hpp"
#include <cstddef>
#include <cstdint>
#include <future>
#include <iostream>
#include <ucontext.h>
#include <vector>

template <typename Traits, typename F> class Context final : public IContext {
  public:
    Context(Traits::SchedulerT *scheduler, size_t stack_size, F &&callable)
        : stack_(stack_size), callable_(std::forward<F>(callable)), scheduler_(scheduler) {
        int ret = getcontext(&context_);
        PROD_ASSERT(ret == 0)
        context_.uc_stack.ss_sp = &stack_[0];
        context_.uc_stack.ss_size = stack_size;
        makecontext(&context_, (void (*)()) Invoke, 1, this);
    }

    bool IsActive() const override { return is_active_; }

    void Switch() override {
        ucontext_t main_context;
        main_context_ = &main_context;
        int ret = swapcontext(main_context_, &context_);
        PROD_ASSERT(ret == 0);
        (*switch_back_task_)();
        switch_back_task_ = nullptr;
    }

    void SwitchBack(SwitchBackTask *task) override {
        auto const *local_main_context = main_context_;
        main_context_ = nullptr;
        switch_back_task_ = task;
        int ret = swapcontext(&context_, local_main_context);
        PROD_ASSERT(ret == 0);
    }

  private:
    static void Invoke(Context *context) {
        typename Traits::FiberT fiber(context->scheduler_, context);
        context->callable_(&fiber);
        context->is_active_ = false;
        setcontext(context->main_context_);
        DIE();   // unreachable
    }

  private:
    ucontext_t context_{};
    ucontext_t *main_context_ = nullptr;
    std::vector<uint8_t> stack_;
    F callable_;
    bool is_active_ = true;
    Traits::SchedulerT *scheduler_ = nullptr;
    SwitchBackTask *switch_back_task_ = nullptr;
};