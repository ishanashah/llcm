#pragma once

#include "lib/concurrent_queue.hpp"
#include <cstddef>
#include <cstdint>
#include <vector>

template <typename Traits> class StackFactoryPool {
  public:
    StackFactoryPool(size_t capacity, size_t stack_size)
        : stack_size_(stack_size), queue_(capacity), pool_(capacity * stack_size) {
        for (size_t i = 0; i < pool_.size(); i += stack_size) {
            queue_.Push(&pool_[i]);
        }
    }

    class Stack {
      public:
        ~Stack() {
            if (stack_ != nullptr) {
                stack_factory_->queue_.Push(stack_);
                stack_ = nullptr;
            }
        }
        Stack(Stack const &) = delete;
        Stack &operator=(Stack const &) = delete;
        constexpr Stack(Stack &&other) noexcept
            : stack_factory_(other.stack_factory_), stack_(other.stack_),
              stack_ptr_(other.stack_ptr_) {
            other.stack_factory_ = nullptr;
            other.stack_ = nullptr;
        }
        constexpr Stack &operator=(Stack &&other) noexcept {
            if (this != &other) {
                Stack tmp(std::move(other));
                std::swap(stack_factory_, tmp.stack_factory_);
                std::swap(stack_, tmp.stack_);
                std::swap(stack_ptr_, tmp.stack_ptr_);
            }
            return *this;
        }

        uint8_t *top() { return stack_; }
        uint8_t *bottom() { return stack_ptr_; }
        size_t size() const { return stack_factory_->stack_size_; }
        template <typename T, typename... Args> void push(Args &&...args) {
            stack_ptr_ -= sizeof(T);
            new (stack_ptr_) T(std::forward<Args>(args)...);
        }
        template <typename T> void pop() {
            T *ptr = reinterpret_cast<T *>(stack_ptr_);
            ptr->~T();
            stack_ptr_ += sizeof(T);
        }

      private:
        friend StackFactoryPool;
        Stack(StackFactoryPool *stack_factory)
            : stack_factory_(stack_factory), stack_(stack_factory->queue_.ForcePop()),
              stack_ptr_(&stack_[stack_factory_->stack_size_ - 1]) {}
        StackFactoryPool *stack_factory_ = nullptr;
        uint8_t *stack_ = nullptr;
        uint8_t *stack_ptr_ = nullptr;
    };

    Stack allocate() { return Stack(this); }

  private:
    size_t stack_size_ = 0;
    ConcurrentQueue<uint8_t *> queue_;
    std::vector<uint8_t> pool_;
};