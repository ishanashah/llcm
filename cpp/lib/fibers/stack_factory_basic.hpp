#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

template <typename Traits> class StackFactoryBasic {
  public:
    StackFactoryBasic(size_t capacity, size_t stack_size) : stack_size_(stack_size) {}
    class Stack {
      public:
        Stack(Stack const &) = delete;
        Stack &operator=(Stack const &) = delete;
        constexpr Stack(Stack &&) = default;
        constexpr Stack &operator=(Stack &&) = default;

        uint8_t *top() { return stack_.data(); }
        uint8_t *bottom() { return &stack_.data()[size() - 1]; }
        size_t size() const { return stack_.size(); }

      private:
        friend StackFactoryBasic;
        Stack(size_t stack_size_) : stack_(stack_size_) {}
        std::vector<uint8_t> stack_;
    };
    Stack allocate() { return Stack(stack_size_); }

  private:
    size_t stack_size_;
};