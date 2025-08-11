#include "lib/callable.hpp"
#include "lib/coroutine.hpp"
#include "lib/scheduler.hpp"

#include <cassert>
#include <iostream>
#include <optional>
#include <stdio.h>

static constexpr size_t SCHEDULER_CAPACITY = 100;
static constexpr size_t STACK_SIZE = 1024 * 16;

struct Callable final : public ICallable {
    virtual void operator()(Coroutine *coroutine) override {
        std::cout << "CALLED CALLABLE ONCE" << std::endl;
        coroutine->Yeild();
        std::cout << "CALLED CALLABLE AGAIN" << std::endl;
    }
};

int main() {
    Scheduler<Coroutine> scheduler(SCHEDULER_CAPACITY, STACK_SIZE);
    std::cout << "Context is " << sizeof(Context<Scheduler<Coroutine>, Coroutine>) << "bytes"
              << std::endl;

    Callable callable;
    scheduler.Schedule(&callable);
    std::cout << "CALLING POLL" << std::endl;
    scheduler.Poll();
    std::cout << "CALLING POLL AGAIN" << std::endl;
    scheduler.Poll();
    std::cout << "SUCCESS" << std::endl;
}