#pragma once

#include "context.hpp"
#include "scheduler.hpp"

class Coroutine {
  public:
    Coroutine(Scheduler *scheduler, Context *context) : scheduler_(scheduler), context_(context) {}

  private:
    Scheduler *scheduler_ = nullptr;
    Context *context_ = nullptr;
};