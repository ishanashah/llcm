#pragma once

#include "callable.hpp"
#include "utils.hpp"
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <ucontext.h>
#include <vector>

class Context {
  public:
    Context() : Context(nullptr, 0, nullptr) {}

    Context(Context *parent, size_t stack_size, ICallable *callable)
        : stack_(stack_size), callable_(callable) {
        int ret = getcontext(&context_);
        PROD_ASSERT(ret == 0)
        if (callable != nullptr) {
            context_.uc_stack.ss_sp = &stack_[0];
            context_.uc_stack.ss_size = stack_size;
            makecontext(&context_, (void (*)()) Invoke, 1, this);
        }
    }

    bool IsActive() const { return callable_ != nullptr; }

    void Swap(Context &other) {
        context_.uc_link = &other.context_;
        other.context_.uc_link = &context_;
        int ret = swapcontext(&context_, &other.context_);
        std::cout << "ISHAN RETURNING FROM SWAP CONTEXT" << std::endl;
        PROD_ASSERT(ret == 0);
    }

    void Yield() {
        int ret = swapcontext(&context_, context_.uc_link);
        PROD_ASSERT(ret == 0);
    }

  private:
    void operator()() {
        (*callable_)();
        std::cout << "ISHAN FINISHED CALLABLE" << std::endl;
        callable_ = nullptr;
        Yield();
    }

    static void Invoke(Context *context) { (*context)(); }

  private:
    ucontext_t context_;
    std::vector<uint8_t> stack_;
    ICallable *callable_ = nullptr;
};