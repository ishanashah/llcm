#pragma once

#include "i_context.hpp"
#include "lib/utils.hpp"
#include <boost/context/detail/fcontext.hpp>
#include <cstddef>
#include <cstdint>
#include <future>
#include <iostream>
#include <ucontext.h>
#include <vector>

template <typename Traits, typename F> class FContext final : public IContext {
  public:
    FContext(Traits::SchedulerT *scheduler, size_t stack_size, F &&callable)
        : stack_(stack_size), callable_(std::forward<F>(callable)), scheduler_(scheduler) {
        context_ =
            boost::context::detail::make_fcontext(&stack_[stack_size - 1], stack_size, Invoke);
    }

    bool IsActive() const override { return is_active_; }

    void Switch() override {
        boost::context::detail::transfer_t transfer =
            boost::context::detail::jump_fcontext(context_, this);
        context_ = transfer.fctx;
        SwitchBackTask *task = static_cast<decltype(task)>(transfer.data);
        (*task)();
    }

    void SwitchBack(SwitchBackTask *task) override {
        main_context_ = boost::context::detail::jump_fcontext(main_context_, task).fctx;
    }

  private:
    static void Invoke(boost::context::detail::transfer_t transfer) {
        FContext *context = static_cast<decltype(context)>(transfer.data);
        context->main_context_ = transfer.fctx;
        typename Traits::FiberT fiber(context->scheduler_, context);
        context->callable_(&fiber);
        context->is_active_ = false;
        SwitchBackTaskWrapper task([]() {});
        context->SwitchBack(&task);
        DIE();   // unreachable
    }

  private:
    boost::context::detail::fcontext_t context_{};
    boost::context::detail::fcontext_t main_context_{};
    std::vector<uint8_t> stack_;
    F callable_;
    bool is_active_ = true;
    Traits::SchedulerT *scheduler_ = nullptr;
};