// xec
// Copyright (c) 2020-2021 Borislav Stanimirov
//
// Distributed under the MIT Software License
// See accompanying file LICENSE.txt or copy at
// https://opensource.org/licenses/MIT
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
namespace internal
{

class XEC_API ThreadExecutionContext final : public ExecutionContext
{
public:
    ThreadExecutionContext();

    // run status
    // both funcs are safe to call from any thread
    bool running() const { return m_running; }
    virtual void stop(ExecutorBase& e) override;

    // wakes up from waiting
    // safe to call from any thread
    // safe to call no matter if the executable is waiting or not
    void wakeUpNow();
    virtual void wakeUpNow(ExecutorBase& e) override;

    // shedule a wake up
    // safe to call from any thread
    // safe to call no matter if the executable is waiting or not
    void scheduleNextWakeUp(std::chrono::milliseconds timeFromNow);
    virtual void scheduleNextWakeUp(ExecutorBase& e, std::chrono::milliseconds timeFromNow) override;
    void unscheduleNextWakeUp();
    virtual void unscheduleNextWakeUp(ExecutorBase& e) override;

    // call at the beginning of each frame
    // will block until woken up
    void wait();

private:
    std::atomic_bool m_running = false;

    // wait state
    bool m_hasWork = false;
    std::optional<std::chrono::steady_clock::time_point> m_scheduledWakeUpTime;
    std::condition_variable m_workCV;
    std::mutex m_workMutex;
};

} // namespace detail

class XEC_API ThreadExecution
{
public:
    // call the following on the main thread
    ThreadExecution(ExecutorBase& e);
    ~ThreadExecution();

    void launchThread(std::optional<std::string_view> threadName = std::nullopt);
    void stopAndJoinThread();

    std::thread::id threadId() const { return m_thread.get_id(); }
private:
    ExecutorBase& m_executor;
    internal::ThreadExecutionContext m_execution;
    ExecutionContext* m_oldExecitionContext;
    std::thread m_thread;

    // NOT MAIN THREAD
    void thread();
};

}
