#include "lib/callable.hpp"
#include "lib/scheduler.hpp"

#include <cassert>
#include <iostream>
#include <optional>
#include <stdio.h>

static constexpr size_t SCHEDULER_CAPACITY = 100;
static constexpr size_t STACK_SIZE = 1024 * 16;

struct Callable final : public ICallable {
    virtual void operator()() override { std::cout << "CALLED CALLABLE" << std::endl; }
};

int main() {
    Scheduler scheduler(SCHEDULER_CAPACITY, STACK_SIZE);
    std::cout << "Context is " << sizeof(Context) << "bytes" << std::endl;

    Callable callable;
    auto coroutine = scheduler.CreateCoroutine(&callable);
    scheduler.Schedule(std::move(coroutine));
    std::cout << "CALLING POLL" << std::endl;
    scheduler.Poll();
    std::cout << "SUCCESS" << std::endl;
}