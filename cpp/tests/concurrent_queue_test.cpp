#include "lib/concurrent_queue.hpp"

#include <cassert>
#include <optional>
#include <pthread.h>
#include <stdio.h>

void basic_queue_test() {
    size_t const capacity = 16;
    ConcurrentQueue<uint64_t> queue(capacity);
    assert(capacity == queue.GetCapacity());

    for (uint64_t i = 1; i <= capacity * 4; i++) {
        bool const reserve_result = queue.TryReserveSizeBeforePush(1);
        assert(reserve_result);
        queue.Push(i);
        uint64_t const pop_result = queue.TryPop().value();
        assert(pop_result == i);
        queue.UnreserveSizeAfterPop(1);
    }

    // reserve capacity entries to push
    for (uint64_t i = 1; i <= capacity; i++) {
        bool const reserve_result = queue.TryReserveSizeBeforePush(1);
        assert(reserve_result);
    }

    // reserving capacity + 1th entry should fail
    {
        bool const reserve_result = queue.TryReserveSizeBeforePush(1);
        assert(!reserve_result);
    }

    // reserve capacity entries at once
    {
        queue.UnreserveSizeAfterPop(capacity);
        bool const reserve_result = queue.TryReserveSizeBeforePush(capacity);
        assert(reserve_result);
    }

    for (uint64_t i = 1; i <= capacity; i++) {
        queue.Push(i);
    }
    for (uint64_t i = 1; i <= capacity; i++) {
        uint64_t const pop_result = queue.TryPop().value();
        assert(pop_result == i);
    }
    assert(std::nullopt == queue.TryPop());
    queue.UnreserveSizeAfterPop(capacity);

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
    ConcurrentQueue<uint64_t> *queue_ = nullptr;
    SequenceTracker *sequence_tracker_ = nullptr;
};

void *thread_exec(void *arg0) {
    struct thread_args *args = reinterpret_cast<struct thread_args *>(arg0);
    for (uint64_t sequence_number = 1; sequence_number < MULTITHREADED_TEST_MAX_SEQUENCE;
         sequence_number++) {
        args->queue_->Push(sequence_number);
        std::optional<uint64_t> pop_result = std::nullopt;
        while (std::nullopt == pop_result) {
            pop_result = args->queue_->TryPop();
        }
        args->sequence_tracker_->MarkObserved(pop_result.value());
    }
    return NULL;
}

void multithreaded_test() {
    ConcurrentQueue<uint64_t> queue(MULTITHREADED_TEST_NUM_THREADS);
    bool const was_reserved = queue.TryReserveSizeBeforePush(MULTITHREADED_TEST_NUM_THREADS);
    assert(was_reserved);
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

    queue.UnreserveSizeAfterPop(MULTITHREADED_TEST_NUM_THREADS);
    sequence_tracker.Validate();
    printf("PASSED multithreaded_test\n");
}

int main() {
    basic_queue_test();
    multithreaded_test();
}