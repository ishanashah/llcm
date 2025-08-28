#include "lib/fibers/buffered_channel.hpp"
#include "lib/fibers/condition_variable.hpp"
#include "lib/fibers/fiber.hpp"
#include "lib/fibers/mutex.hpp"
#include "lib/fibers/scheduler.hpp"
#include "lib/fibers/semaphore.hpp"
#include "lib/fibers/traits.hpp"
#include "lib/fibers/unbuffered_channel.hpp"

#include <cassert>
#include <iostream>
#include <optional>
#include <stdio.h>
#include <thread>

static constexpr size_t SCHEDULER_CAPACITY = 100;
static constexpr size_t STACK_SIZE = 1024 * 16;

struct ThreadArgs {
    size_t tid_ = 0;
    Scheduler<Traits> *scheduler = nullptr;
};

void thread_function(std::stop_token stoken, ThreadArgs args) {
    Scheduler<Traits> *scheduler = args.scheduler;
    for (auto i = 0; i < 10000; i++) {
        bool success = false;
        do {
            success = scheduler->Poll();
        } while (!success);
    }
}

struct Callable {
    void operator()(Fiber<Traits> *fiber) {
        std::cout << "BEGIN CALLABLE" << std::endl;
        fiber->Yeild();
        std::cout << "MIDDLE CALLABLE" << std::endl;
        fiber->Yeild();
        std::cout << "END CALLABLE" << std::endl;
    }

    ~Callable() { std::cout << "CALLABLE DESTRUCTOR" << std::endl; }
};

int main() {
    Scheduler<Traits> scheduler(SCHEDULER_CAPACITY, STACK_SIZE);
    uint64_t shared_counter = 0;
    Mutex<Traits> mutex;
    ConditionVariable<Traits> condition_variable;
    (void) condition_variable;
    UnbufferedChannel<Traits, uint64_t> channel;
    BufferedChannel<Traits, int> buffered_channel(5);

    auto const lambda = [&](Fiber<Traits> *fiber) {
        while (true) {
            uint64_t local_counter = 0;
            mutex.Lock(fiber);
            local_counter = shared_counter + 1;
            shared_counter = local_counter;
            mutex.Unlock();
            if (local_counter % 2 == 0) {
                auto received_counter = channel.Receive(fiber);
                assert(received_counter == local_counter - 1);
            } else {
                channel.Send(fiber, local_counter);
            }
        }
    };
    scheduler.Schedule(lambda);
    scheduler.Schedule(lambda);

    Callable callable;
    scheduler.Schedule(std::move(callable));

    ThreadArgs args0{.tid_ = 0, .scheduler = &scheduler};
    ThreadArgs args1{.tid_ = 1, .scheduler = &scheduler};
    {
        std::jthread my_jthread0(thread_function, args0);
        std::jthread my_jthread1(thread_function, args1);
    }
    std::cout << "Shared counter " << shared_counter << std::endl;
}