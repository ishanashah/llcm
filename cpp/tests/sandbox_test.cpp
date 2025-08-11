#include "lib/callable.hpp"
#include "lib/coroutine.hpp"
#include "lib/mutex.hpp"
#include "lib/scheduler.hpp"

#include <cassert>
#include <iostream>
#include <optional>
#include <stdio.h>
#include <thread>

static constexpr size_t SCHEDULER_CAPACITY = 100;
static constexpr size_t STACK_SIZE = 1024 * 16;

struct Callable final : public ICallable {
    virtual void operator()(Coroutine *coroutine) override {
        while (true) {
            counter_ += 1;
            coroutine->Yeild();
        }
    }

    uint64_t counter_ = 0;
};

struct ThreadArgs {
    size_t tid_ = 0;
    Scheduler<Coroutine> *scheduler = nullptr;
};

void worker_function(std::stop_token stoken, ThreadArgs args) {
    Scheduler<Coroutine> *scheduler = args.scheduler;
    for (auto i = 0; i < 10000; i++) {
        bool success = false;
        do {
            success = scheduler->Poll();
        } while (!success);
    }
}

int main() {
    Scheduler<Coroutine> scheduler(SCHEDULER_CAPACITY, STACK_SIZE);
    Callable callable0;
    scheduler.Schedule(&callable0);
    Callable callable1;
    scheduler.Schedule(&callable1);
    ThreadArgs args0{.tid_ = 0, .scheduler = &scheduler};
    ThreadArgs args1{.tid_ = 1, .scheduler = &scheduler};
    {
        std::jthread my_jthread0(worker_function, args0);
        std::jthread my_jthread1(worker_function, args1);
    }
    std::cout << "Callable0 invoked " << callable0.counter_ << " times" << std::endl;
    std::cout << "Callable1 invoked " << callable1.counter_ << " times" << std::endl;
}