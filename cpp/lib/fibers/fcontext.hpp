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

template <typename Traits> class FContext final : public IContext {
  public:
    template <typename F>
    FContext(Traits::SchedulerT *scheduler, size_t stack_size, F &&callable)
        : stack_(stack_size), context_(boost::context::detail::make_fcontext(
                                  &stack_[stack_size - 1], stack_size, CallableWrapper<F>::Invoke)),
          scheduler_(scheduler) {
        CallableWrapper<F> tmp_wrapper(std::forward<F>(callable), this);
        context_ = boost::context::detail::jump_fcontext(context_, &tmp_wrapper).fctx;
    }

    bool IsActive() const override { return is_active_; }

    void Switch() override {
        boost::context::detail::transfer_t transfer =
            boost::context::detail::jump_fcontext(context_, nullptr);
        context_ = transfer.fctx;
        SwitchBackTask *task = static_cast<decltype(task)>(transfer.data);
        (*task)();
    }

    void SwitchBack(SwitchBackTask *task) override {
        main_context_ = boost::context::detail::jump_fcontext(main_context_, task).fctx;
    }

  private:
    template <typename F> struct CallableWrapper {
        CallableWrapper(F &&callable, FContext *context)
            : callable_(std::forward<F>(callable)), context_(context) {}
        F callable_;
        FContext *context_;

        static void Invoke(boost::context::detail::transfer_t transfer) {
            CallableWrapper *tmp_wrapper = static_cast<decltype(tmp_wrapper)>(transfer.data);
            CallableWrapper wrapper = std::move(*tmp_wrapper);
            auto *context = wrapper.context_;
            context->main_context_ =
                boost::context::detail::jump_fcontext(transfer.fctx, nullptr).fctx;
            typename Traits::FiberT fiber(context->scheduler_, context);
            wrapper.callable_(&fiber);
            context->is_active_ = false;
            SwitchBackTaskWrapper task([]() {});
            context->SwitchBack(&task);
            DIE();   // unreachable
        }
    };

  private:
    std::vector<uint8_t> stack_;
    boost::context::detail::fcontext_t context_{};
    boost::context::detail::fcontext_t main_context_{};
    bool is_active_ = true;
    Traits::SchedulerT *scheduler_ = nullptr;
};