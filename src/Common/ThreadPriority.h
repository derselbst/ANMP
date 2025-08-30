#pragma once

#include <memory>

/**
  * class ThreadPriority
  *
  */

enum class Priority
{
    High
};

class ThreadPriority
{
    public:
    ThreadPriority(Priority p);
    ~ThreadPriority();
    // no copy
    ThreadPriority(const ThreadPriority &) = delete;
    // no assign
    ThreadPriority &operator=(const ThreadPriority &) = delete;

    private:
        struct Impl;
        std::unique_ptr<Impl> d;
};
