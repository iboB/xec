// Copyright (c) Borislav Stanimirov
// SPDX-License-Identifier: MIT
//
#pragma once
#include "ExecutionContext.hpp"

#include <mutex>
#include <atomic>
#include <condition_variable>
#include <thread>
#include <string_view>
#include <optional>

namespace xec
{

class XEC_API ThreadExecutionContext final : public ExecutionContext
{
public:
    ThreadExecutionContext();

    // run status
    // both funcs are safe to call from any thread
    virtual bool running() const override { return m_running; }
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

    // call at the beginning of each frame
    // will block until woken up
    void wait();

private:
    std::atomic_bool m_running;

    // wait state
    bool m_hasWork;
    std::optional<std::chrono::steady_clock::time_point> m_scheduledWakeUpTime;
    std::condition_variable m_workCV;
    std::mutex m_workMutex;
};

class XEC_API ThreadExecution
{
public:
    // call the following on the main thread
    ThreadExecution(ExecutorBase& e);
    ~ThreadExecution();

    void launchThread(std::optional<std::string_view> threadName = std::nullopt);
    void joinThread(); // Wait for thread to join. WARNING: unless someone stops the execution, this will wait indefinitely!
    void stopAndJoinThread(); // Stop the execution and wait for join

    std::thread::id threadId() const { return m_thread.get_id(); }
private:
    ExecutorBase& m_executor;
    ThreadExecutionContext* m_context = nullptr;
    std::thread m_thread;

    // NOT MAIN THREAD
    void thread();
};

}
