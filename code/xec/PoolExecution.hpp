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

class PoolExecutionContext : public ExecutionContext {
public:
    // run status
    // both funcs are safe to call from any thread
    bool running() const override { return m_running; }
    virtual void stop() override;

    // wakes up from waiting
    // safe to call from any thread
    // safe to call no matter if the executable is waiting or not
    void wakeUpNow() override;

    // shedule a wake up
    // safe to call from any thread
    // safe to call no matter if the executable is waiting or not
    void scheduleNextWakeUp(std::chrono::milliseconds timeFromNow) override;
    void unscheduleNextWakeUp() override;

private:
    std::atomic_bool m_running = false;
};

class PoolExecution {
public:
    PoolExecution(uint32_t numThreads);
    ~PoolExecution();

    void addExecutor(ExecutorBase& executor);

    void launchThreads();
    void joinThreads();
public:
    struct Strand {
        PoolExecutionContext* context;
        ExecutorBase& executor;
    };
    std::mutex m_strandMutex;
    std::vector<Strand> m_strands;

    void thread();
    std::vector<std::thread> m_threads;
    std::atomic_bool m_stopping;
};

}
