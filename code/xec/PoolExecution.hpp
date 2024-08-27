// Copyright (c) Borislav Stanimirov
// SPDX-License-Identifier: MIT
//
#pragma once
#include "API.h"
#include <memory>
#include <cstdint>
#include <optional>
#include <string_view>

namespace xec {
class ExecutorBase;

class XEC_API PoolExecution {
public:
    PoolExecution();
    ~PoolExecution();

    void addExecutor(ExecutorBase& executor); // valid on any thread
    void stop(); // valid on any thread

    // these three functions must be called on the same thread
    void launchThreads(size_t count, std::optional<std::string_view> threadName = {});
    void joinThreads();
    void stopAndJoinThreads();

    void run(); // blocks current thread with a worker loop

public:
    class Impl;
    std::unique_ptr<Impl> m_impl;

    class Context;
};

}
