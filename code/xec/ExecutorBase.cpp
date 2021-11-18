// xec
// Copyright (c) 2020-2021 Borislav Stanimirov
//
// Distributed under the MIT Software License
// See accompanying file LICENSE.txt or copy at
// https://opensource.org/licenses/MIT
//
#include "ExecutorBase.hpp"
#include "ExecutionContext.hpp"

#include <cassert>
#include <new>
#include <optional>

namespace xec
{

namespace
{
auto tnow() { return std::chrono::steady_clock::now(); }
}

class ExecutorBase::InitialContext : public ExecutionContext
{
public:
    virtual void wakeUpNow(xec::ExecutorBase&) override { m_wakeUpNow = true; }
    virtual void stop(xec::ExecutorBase&) override { m_stop = true; }
    virtual void scheduleNextWakeUp(xec::ExecutorBase&, std::chrono::milliseconds timeFromNow) override
    {
        m_scheduledWakeUpTime = tnow() + timeFromNow;
    }
    virtual void unscheduleNextWakeUp(xec::ExecutorBase&) override
    {
        m_scheduledWakeUpTime.reset();
    }

    // transfer stored data (if any) to newly set execution context of executor
    void transfer(ExecutorBase& e)
    {
        if (m_stop)
        {
            e.stop();
        }
        else if (m_wakeUpNow)
        {
            e.wakeUpNow();
        }
        else if (m_scheduledWakeUpTime)
        {
            auto diff = *m_scheduledWakeUpTime - tnow();
            if (diff.count() <= 0)
            {
                e.wakeUpNow();
            }
            else
            {
                auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(diff);
                e.scheduleNextWakeUp(ms);
            }
        }
    }

    bool m_stop = false;
    bool m_wakeUpNow = false;
    std::optional<std::chrono::steady_clock::time_point> m_scheduledWakeUpTime;
};

ExecutorBase::ExecutorBase()
    : m_initialContext(new (&m_initialContextBuffer) InitialContext)
{
    static_assert(sizeof(InitialContext) <= sizeof(m_initialContextBuffer));
    m_executionContext = m_initialContext;
}

ExecutorBase::ExecutorBase(ExecutionContext& context)
    : m_executionContext(&context)
{}

ExecutorBase::~ExecutorBase()
{
    if (m_initialContext) m_initialContext->~InitialContext();
}

void ExecutorBase::setExecutionContext(ExecutionContext& context)
{
    assert(m_executionContext == m_initialContext);
    m_executionContext = &context;

    m_initialContext->transfer(*this);
    m_initialContext->~InitialContext();
    m_initialContext = nullptr;
}

void ExecutorBase::wakeUpNow()
{
    m_executionContext->wakeUpNow(*this);
}

void ExecutorBase::scheduleNextWakeUp(std::chrono::milliseconds timeFromNow)
{
    m_executionContext->scheduleNextWakeUp(*this, timeFromNow);
}

void ExecutorBase::unscheduleNextWakeUp()
{
    m_executionContext->unscheduleNextWakeUp(*this);
}

void ExecutorBase::stop()
{
    m_executionContext->stop(*this);
}

// export ExecutionContext vtable;
ExecutionContext::~ExecutionContext() = default;

}
