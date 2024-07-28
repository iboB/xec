// Copyright (c) Borislav Stanimirov
// SPDX-License-Identifier: MIT
//
#include "ThreadExecution.hpp"

#include "ExecutorBase.hpp"
#include "ThreadName.hpp"

#include <cassert>
#include <functional>

namespace xec {

ThreadExecutionContext::ThreadExecutionContext()
    : m_running(true)
    , m_hasWork(true)
{}

void ThreadExecutionContext::stop() {
    m_running = false;
    wakeUpNow();
}

void ThreadExecutionContext::wakeUpNow() {
    {
        std::lock_guard<std::mutex> lk(m_workMutex);
        m_hasWork = true;
    }
    m_workCV.notify_one();
}

void ThreadExecutionContext::scheduleNextWakeUp(ms_t timeFromNow) {
    {
        std::lock_guard<std::mutex> lk(m_workMutex);
        m_scheduledWakeUpTime = clock_t::now() + timeFromNow;
    }
    m_workCV.notify_one();
}

void ThreadExecutionContext::unscheduleNextWakeUp() {
    {
        std::lock_guard<std::mutex> lk(m_workMutex);
        m_scheduledWakeUpTime.reset();
    }
    m_workCV.notify_one();
}

void ThreadExecutionContext::wait() {
    std::unique_lock<std::mutex> lock(m_workMutex);

    while (true) {
        if (m_hasWork) {
            m_hasWork = false;
            m_scheduledWakeUpTime.reset(); // forget about scheduling wakeup if we were woken up with work to do
            return;
        }

        if (m_scheduledWakeUpTime) {
            // wait until if we have a wake up time, wait for it
            auto status = m_workCV.wait_until(lock, *m_scheduledWakeUpTime);

            if (status == std::cv_status::timeout) {
                // timeout => hasWork
                m_hasWork = true;
                m_scheduledWakeUpTime.reset(); // timer was consumed
            }

            // otherwise we should do nothing... we either have work and will return
            // or we don't have work and will loop again

            // this might be a spurious wake up
            // in such case we don't have work, and don't have a new timer
            // so we will loop again and wait on the same timer again
            // it may also be the case that we've been woken up by unschedule
            // in this case we loop again and will wait indefinitely
        }
        else {
            // wait indefinitely
            // when we wake up we either have work and will return
            // or will have a scheduled wake up time and will wait for it
            // or it's a spurious wake up and will end up here again
            m_workCV.wait(lock);
        }
    }
}

LocalExecution::LocalExecution(ExecutorBase& e)
    : m_executor(e)
{
    auto ctx = std::make_unique<ThreadExecutionContext>();
    m_context = ctx.get();
    m_executor.setExecutionContext(std::move(ctx));
}

void LocalExecution::run() {
    while (m_context->running()) {
        m_context->wait();
        m_executor.update();
    }
    m_executor.finalize();
}

ThreadExecution::ThreadExecution(ExecutorBase& e)
    : LocalExecution(e)
{}

ThreadExecution::~ThreadExecution() {
    stopAndJoinThread();
}

void ThreadExecution::launchThread(std::optional<std::string_view> threadName) {
    assert(!m_thread.joinable()); // we have an active thread???
    m_thread = std::thread(std::bind(&ThreadExecution::thread, this));
    if (threadName) {
        SetThreadName(m_thread, *threadName);
    }
}

void ThreadExecution::joinThread() {
    if (m_thread.joinable()) {
        m_thread.join();
    }
}

void ThreadExecution::stopAndJoinThread() {
    m_executor.stop();
    joinThread();
}

void ThreadExecution::thread() {
    LocalExecution::run();
}

}
