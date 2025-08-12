#pragma once

#include <utility>
struct SwitchBackTask {
    virtual void operator()() = 0;
    virtual ~SwitchBackTask() = default;
};

template <typename F> struct SwitchBackTaskWrapper final : public SwitchBackTask {
    SwitchBackTaskWrapper(F &&function) : function_(std::forward<F>(function)) {}
    void operator()() override { function_(); }
    F function_;
};

struct IContext {
    virtual bool IsActive() const = 0;
    virtual void Switch() = 0;
    virtual void SwitchBack(SwitchBackTask *) = 0;
    virtual ~IContext() = default;
};
