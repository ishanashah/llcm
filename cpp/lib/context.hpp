#pragma once

#include "callable.hpp"
#include "utils.hpp"
#include <cstdint>
#include <iostream>
#include <ucontext.h>
#include <vector>

class Context {
  public:
    Context() {
        int ret = getcontext(&context_);
        PROD_ASSERT(ret == 0);
    }
    Context(void *stack_ptr, size_t stack_size, ICallable *callable) : Context() {
        context_.uc_stack.ss_sp = stack_ptr;
        context_.uc_stack.ss_size = stack_size;
        makecontext(&context_, (void (*)()) Function, 0);
    }

    void Swap(Context &other) {
        int ret = swapcontext(&context_, &other.context_);
        PROD_ASSERT(ret == 0);
    }

  private:
    static void Function(void) {
        std::cout << "ISHAN IN CONTEXT FUNCTION GOING TO CALL CALLABLE" << std::endl;
        // (*callable)();
    }

  private:
    ucontext_t context_;
};