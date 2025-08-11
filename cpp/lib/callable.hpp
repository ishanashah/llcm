#pragma once

class Coroutine;

struct ICallable {
    virtual void operator()(Coroutine *) = 0;
    virtual ~ICallable() = default;
};