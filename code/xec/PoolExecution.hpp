// Copyright (c) Borislav Stanimirov
// SPDX-License-Identifier: MIT
//
#pragma once
#include "ExecutionContext.hpp"

#include <memory>
#include <atomic>
#include <thread>
#include <mutex>

#include <vector>

namespace xec {
class ExecutorBase;

class PoolExecution {
public:
    PoolExecution(uint32_t numThreads);
    ~PoolExecution();

    void addExecutor(ExecutorBase& executor);

    void launchThreads();
    void joinThreads();
public:
    struct Strand {
        //PoolExecutionContext* context;
        ExecutorBase& executor;
    };
    std::mutex m_strandMutex;
    std::vector<Strand> m_strands;

    void thread();
    std::vector<std::thread> m_threads;
    std::atomic_bool m_stopping;
};

}
