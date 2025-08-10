#include "lib/scmp_dynamic_concurrent_queue.hpp"

#include <cassert>
#include <iostream>
#include <optional>
#include <pthread.h>
#include <stdio.h>
#include <vector>

void basic_queue_test() {
    size_t const capacity = 16;
    DynamicConcurrentQueue<uint64_t> queue;

    for (uint64_t i = 1; i <= capacity * 4; i++) {
        queue.Push(new DynamicConcurrentQueueEntry<uint64_t>{.element_ = i});
        auto *pop_result = queue.TryPop();
        assert(pop_result != nullptr);
        assert(pop_result->element_ == i);
        delete pop_result;
    }

    for (uint64_t i = 1; i <= capacity; i++) {
        queue.Push(new DynamicConcurrentQueueEntry<uint64_t>{.element_ = i});
    }
    for (uint64_t i = 1; i <= capacity; i++) {
        auto *pop_result = queue.TryPop();
        assert(pop_result != nullptr);
        assert(pop_result->element_ == i);
        delete pop_result;
    }
    assert(nullptr == queue.TryPop());

    printf("PASSED basic_queue_test\n");
}

#define MULTITHREADED_TEST_NUM_THREADS  32
#define MULTITHREADED_TEST_MAX_SEQUENCE 1024

class SequenceTracker {
  public:
    void MarkObserved(uint64_t sequence_number) {
        __atomic_fetch_add(&num_observations_[sequence_number], 1, __ATOMIC_SEQ_CST);
    }
    void Validate() {
        for (uint64_t sequence = 1; sequence < MULTITHREADED_TEST_MAX_SEQUENCE; sequence++) {
            assert(num_observations_[sequence] == MULTITHREADED_TEST_NUM_THREADS);
        }
    }

  private:
    uint64_t num_observations_[MULTITHREADED_TEST_MAX_SEQUENCE] = {};
};

struct thread_args {
    DynamicConcurrentQueue<uint64_t> *queue_ = nullptr;
    SequenceTracker *sequence_tracker_ = nullptr;
};

void *thread_exec(void *arg0) {
    struct thread_args *args = reinterpret_cast<struct thread_args *>(arg0);
    for (uint64_t sequence_number = 1; sequence_number < MULTITHREADED_TEST_MAX_SEQUENCE;
         sequence_number++) {
        args->queue_->Push(new DynamicConcurrentQueueEntry<uint64_t>{.element_ = sequence_number});
        DynamicConcurrentQueueEntry<uint64_t> *pop_result = nullptr;
        while (nullptr == pop_result) {
            pop_result = args->queue_->TryPop();
        }
        args->sequence_tracker_->MarkObserved(pop_result->element_);
    }
    return NULL;
}

void multithreaded_test() {
    DynamicConcurrentQueue<uint64_t> queue;
    SequenceTracker sequence_tracker;

    struct thread_args thread_args = {.queue_ = &queue, .sequence_tracker_ = &sequence_tracker};
    pthread_t threads[MULTITHREADED_TEST_NUM_THREADS];
    for (size_t tid = 0; tid < MULTITHREADED_TEST_NUM_THREADS; tid++) {
        int rc = pthread_create(&threads[tid], NULL, thread_exec, &thread_args);
        assert(rc == 0);
    }

    for (size_t tid = 0; tid < MULTITHREADED_TEST_NUM_THREADS; tid++) {
        pthread_join(threads[tid], NULL);
    }

    sequence_tracker.Validate();
    printf("PASSED multithreaded_test\n");
}

int main() {
    basic_queue_test();
    multithreaded_test();
}