#pragma once

struct ICallable {
    virtual void operator()() = 0;
    virtual ~ICallable() = default;
};