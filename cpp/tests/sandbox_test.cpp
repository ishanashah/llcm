#include "lib/coroutine.hpp"
#include "lib/mutex.hpp"
#include "lib/scheduler.hpp"
#include "lib/traits.hpp"

#include <cassert>
#include <iostream>
#include <optional>
#include <stdio.h>
#include <thread>

static constexpr size_t SCHEDULER_CAPACITY = 100;
static constexpr size_t STACK_SIZE = 1024 * 16;

struct Callable {
    void operator()(Coroutine<Traits> *coroutine) {
        while (true) {
            counter_ += 1;
            mutex_->Lock(coroutine);
            *shared_counter_ += 1;
            mutex_->Unlock();
            coroutine->Yeild();
        }
    }

    uint64_t counter_ = 0;
    uint64_t *shared_counter_ = 0;
    Mutex<Traits> *mutex_ = nullptr;
};

struct ThreadArgs {
    size_t tid_ = 0;
    Scheduler<Traits> *scheduler = nullptr;
};

void worker_function(std::stop_token stoken, ThreadArgs args) {
    Scheduler<Traits> *scheduler = args.scheduler;
    for (auto i = 0; i < 10000; i++) {
        bool success = false;
        do {
            success = scheduler->Poll();
        } while (!success);
    }
}

int main() {
    Scheduler<Traits> scheduler(SCHEDULER_CAPACITY, STACK_SIZE);
    uint64_t shared_counter = 0;
    Callable callable0;
    callable0.shared_counter_ = &shared_counter;
    scheduler.Schedule(callable0);
    Callable callable1;
    callable1.shared_counter_ = &shared_counter;
    scheduler.Schedule(callable1);
    Mutex<Traits> mutex;
    callable0.mutex_ = &mutex;
    callable1.mutex_ = &mutex;
    ThreadArgs args0{.tid_ = 0, .scheduler = &scheduler};
    ThreadArgs args1{.tid_ = 1, .scheduler = &scheduler};
    {
        std::jthread my_jthread0(worker_function, args0);
        std::jthread my_jthread1(worker_function, args1);
    }
    std::cout << "Callable0 invoked " << callable0.counter_ << " times" << std::endl;
    std::cout << "Callable1 invoked " << callable1.counter_ << " times" << std::endl;
    std::cout << "Shared counter " << shared_counter << std::endl;
}