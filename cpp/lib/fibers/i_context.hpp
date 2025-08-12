#pragma once

struct SwitchBackTask {
    virtual void operator()() = 0;
    virtual ~SwitchBackTask() = default;
};

struct IContext {
    virtual bool IsActive() const = 0;
    virtual void Switch() = 0;
    virtual void SwitchBack(SwitchBackTask *) = 0;
    virtual ~IContext() = default;
};
