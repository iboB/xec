// xec
// Copyright (c) 2020-2021 Borislav Stanimirov
//
// Distributed under the MIT Software License
// See accompanying file LICENSE.txt or copy at
// https://opensource.org/licenses/MIT
//
#include "ThreadExecution.hpp"

#include "ExecutorBase.hpp"
#include "ThreadName.hpp"

#include <cassert>
#include <functional>

namespace xec
{
namespace internal
{

ThreadExecutionContext::ThreadExecutionContext()
    : m_running(true)
    , m_hasWork(true)
{}

void ThreadExecutionContext::stop(ExecutorBase&)
{
    m_running = false;
    wakeUpNow();
}

void ThreadExecutionContext::wakeUpNow()
{
    {
        std::lock_guard<std::mutex> lk(m_workMutex);
        m_hasWork = true;
    }
    m_workCV.notify_one();
}

void ThreadExecutionContext::wakeUpNow(ExecutorBase&)
{
    wakeUpNow();
}

void ThreadExecutionContext::scheduleNextWakeUp(std::chrono::milliseconds timeFromNow)
{
    {
        std::lock_guard<std::mutex> lk(m_workMutex);
        m_scheduledWakeUpTime = std::chrono::steady_clock::now() + timeFromNow;
    }
    m_workCV.notify_one();
}

void ThreadExecutionContext::scheduleNextWakeUp(ExecutorBase&, std::chrono::milliseconds timeFromNow)
{
    scheduleNextWakeUp(timeFromNow);
}

void ThreadExecutionContext::unscheduleNextWakeUp()
{
    {
        std::lock_guard<std::mutex> lk(m_workMutex);
        m_scheduledWakeUpTime.reset();
    }
    m_workCV.notify_one();
}

void ThreadExecutionContext::unscheduleNextWakeUp(ExecutorBase&)
{
    unscheduleNextWakeUp();
}

void ThreadExecutionContext::wait()
{
    std::unique_lock<std::mutex> lock(m_workMutex);

    while (true)
    {
        if (m_hasWork)
        {
            m_hasWork = false;
            m_scheduledWakeUpTime.reset(); // forget about scheduling wakeup if we were woken up with work to do
            return;
        }

        if (m_scheduledWakeUpTime)
        {
            // wait until if we have a wake up time, wait for it
            auto status = m_workCV.wait_until(lock, *m_scheduledWakeUpTime);

            if (status == std::cv_status::timeout)
            {
                // timeout => hasWork
                m_hasWork = true;
                m_scheduledWakeUpTime.reset(); // timer was consumed
            }

            // otherwise we should do nothing... we either have work and will return
            // or we don't have work and will loop again

            // this might be a spurious wake up
            // in such case we don't have work, and don't have a new timer
            // so we will loop again and wait on the same timer again
            // it may also be the case tha we've been woken up by unschedule
            // in this case we loop again and will wait indefinitely
        }
        else
        {
            // wait indefinitely
            // when we wake up we either have work and will return
            // or will have a scheduled wake up time and will wait for it
            // or it's a spurious wake up and will end up here again
            m_workCV.wait(lock);
        }
    }
}

} // namespace detail

ThreadExecution::ThreadExecution(ExecutorBase& e)
    : m_executor(e)
{
    m_oldExecitionContext = &m_executor.setExecutionContext(m_execution);
}

ThreadExecution::~ThreadExecution()
{
    stopAndJoinThread();
    m_executor.setExecutionContext(*m_oldExecitionContext);
}

void ThreadExecution::launchThread(std::optional<std::string_view> threadName)
{
    assert(!m_thread.joinable()); // we have an active thread???
    m_thread = std::thread(std::bind(&ThreadExecution::thread, this));
    if (threadName)
    {
        SetThreadName(m_thread, *threadName);
    }
}

void ThreadExecution::stopAndJoinThread()
{
    if (m_thread.joinable())
    {
        m_executor.stop();
        m_thread.join();
        m_executor.finalize();
    }
}

void ThreadExecution::thread()
{
    while(m_execution.running())
    {
        m_execution.wait();
        m_executor.update();
    }
}

}
