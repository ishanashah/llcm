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

struct Callable {
    void operator()(Fiber<Traits> *fiber) {
        while (true) {
            uint64_t local_counter = 0;
            mutex_->Lock(fiber);
            local_counter = *shared_counter_ + 1;
            *shared_counter_ = local_counter;
            mutex_->Unlock();
            if (local_counter % 2 == 0) {
                auto received_counter = channel_->Receive(fiber);
                assert(received_counter == local_counter - 1);
            } else {
                channel_->Send(fiber, local_counter);
            }
        }
    }

    uint64_t counter_ = 0;
    uint64_t *shared_counter_ = nullptr;
    Mutex<Traits> *mutex_ = nullptr;
    ConditionVariable<Traits> *condition_variable_ = nullptr;
    UnbufferedChannel<Traits, uint64_t> *channel_;
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
    ConditionVariable<Traits> condition_variable;
    callable0.condition_variable_ = &condition_variable;
    callable1.condition_variable_ = &condition_variable;
    UnbufferedChannel<Traits, uint64_t> channel;
    callable0.channel_ = &channel;
    callable1.channel_ = &channel;
    BufferedChannel<Traits, int> buffered_channel_(5);
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