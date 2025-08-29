#pragma once

#include "lib/utils.hpp"
#include <boost/context/detail/fcontext.hpp>
#include <cstddef>
#include <cstdint>
#include <future>
#include <ucontext.h>
#include <vector>

template <typename Traits> class FContext {
  public:
    template <typename F>
    FContext(Traits::SchedulerT *scheduler, Traits::StackT stack, F &&callable)
        : stack_(std::move(stack)),
          context_(boost::context::detail::make_fcontext(stack_.bottom(), stack_.size(),
                                                         CallableWrapper<F>::Invoke)) {
        CallableWrapper<F> tmp_wrapper(std::forward<F>(callable), this, scheduler);
        context_ = boost::context::detail::jump_fcontext(context_, &tmp_wrapper).fctx;
    }

    FContext(FContext const &) = delete;
    void operator=(FContext const &) = delete;

    bool IsActive() const { return is_active_; }

    void Switch() {
        boost::context::detail::transfer_t transfer =
            boost::context::detail::jump_fcontext(context_, nullptr);
        context_ = transfer.fctx;
        SwitchBackTask *task = static_cast<decltype(task)>(transfer.data);
        (*task)();
    }

    template <typename F> void SwitchBack(F &&switch_back_task) {
        struct SwitchBackTaskWrapper final : public SwitchBackTask {
            SwitchBackTaskWrapper(F &&function) : function_(std::forward<F>(function)) {}
            void operator()() override { function_(); }
            F function_;
        } wrapper(std::forward<F>(switch_back_task));
        context_ = boost::context::detail::jump_fcontext(context_, &wrapper).fctx;
    }

  private:
    template <typename F> struct CallableWrapper {
        CallableWrapper(F &&callable, FContext *context, Traits::SchedulerT *scheduler)
            : callable_(std::forward<F>(callable)), context_(context), scheduler_(scheduler) {}

        static void Invoke(boost::context::detail::transfer_t transfer) {
            CallableWrapper *tmp_wrapper = static_cast<decltype(tmp_wrapper)>(transfer.data);
            auto *context = tmp_wrapper->context_;
            {
                CallableWrapper wrapper = std::move(*tmp_wrapper);
                context->context_ =
                    boost::context::detail::jump_fcontext(transfer.fctx, nullptr).fctx;
                typename Traits::FiberT fiber(wrapper.scheduler_, context);
                wrapper.callable_(&fiber);
                context->is_active_ = false;
                // make sure to destroy everything
            }
            context->SwitchBack([]() {});
            DIE();   // unreachable
        }

        F callable_;
        FContext *context_;
        Traits::SchedulerT *scheduler_;
    };

    struct SwitchBackTask {
        virtual void operator()() = 0;
        virtual ~SwitchBackTask() = default;
    };

  private:
    Traits::StackT stack_;
    boost::context::detail::fcontext_t context_{};
    bool is_active_ = true;
};