// Copyright (c) Borislav Stanimirov
// SPDX-License-Identifier: MIT
//
#include "ExecutorBase.hpp"
#include "ExecutionContext.hpp"

#include <cassert>
#include <optional>

namespace xec {

class ExecutorBase::InitialContext final : public ExecutionContext {
public:
    virtual void wakeUpNow() override { m_wakeUpNow = true; }
    virtual bool running() const override { return !m_stop; }
    virtual void stop() override { m_stop = true; }
    virtual void scheduleNextWakeUp(ms_t timeFromNow) override {
        m_scheduledWakeUpTime = clock_t::now() + timeFromNow;
    }
    virtual void unscheduleNextWakeUp() override {
        m_scheduledWakeUpTime.reset();
    }

    // transfer stored data (if any) to newly set execution context of executor
    void transfer(ExecutorBase& e) {
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
            auto diff = *m_scheduledWakeUpTime - clock_t::now();
            if (diff.count() <= 0)
            {
                e.wakeUpNow();
            }
            else
            {
                auto ms = std::chrono::duration_cast<ms_t>(diff);
                e.scheduleNextWakeUp(ms);
            }
        }
    }

    bool m_stop = false;
    bool m_wakeUpNow = false;
    std::optional<clock_t::time_point> m_scheduledWakeUpTime;
};

ExecutorBase::ExecutorBase()
    : m_executionContext(std::make_unique<InitialContext>())
    , m_initialContext(static_cast<InitialContext*>(m_executionContext.get()))
{}

ExecutorBase::ExecutorBase(std::unique_ptr<ExecutionContext> context)
    : m_executionContext(std::move(context))
{}

ExecutorBase::~ExecutorBase() = default;

void ExecutorBase::setExecutionContext(std::unique_ptr<ExecutionContext> context) {
    assert(m_executionContext.get() == m_initialContext);
    auto keep = std::move(m_executionContext); // keep initial context alive for the transfer
    m_executionContext = std::move(context);
    m_initialContext->transfer(*this);
}

void ExecutorBase::wakeUpNow() {
    m_executionContext->wakeUpNow();
}

void ExecutorBase::scheduleNextWakeUp(ms_t timeFromNow) {
    m_executionContext->scheduleNextWakeUp(timeFromNow);
}

void ExecutorBase::unscheduleNextWakeUp() {
    m_executionContext->unscheduleNextWakeUp();
}

void ExecutorBase::stop() {
    m_executionContext->stop();
}

// export ExecutionContext vtable;
ExecutionContext::~ExecutionContext() = default;

}
