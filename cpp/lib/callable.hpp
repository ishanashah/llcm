#pragma once

class Coroutine;

struct ICallable {
    virtual void operator()() = 0;
    virtual ~ICallable() = default;
};