// xec
// Copyright (c) 2020-2021 Borislav Stanimirov
//
// Distributed under the MIT Software License
// See accompanying file LICENSE.txt or copy at
// https://opensource.org/licenses/MIT
//
#include "ExecutorBase.hpp"
#include "ExecutionContext.hpp"

namespace xec
{

namespace
{

class NoopExecutionContext final : public ExecutionContext
{
    virtual void wakeUpNow(ExecutorBase&) override {}
    virtual void stop(ExecutorBase&) override {}
    virtual void scheduleNextWakeUp(ExecutorBase&, std::chrono::milliseconds) override {}
    virtual void unscheduleNextWakeUp(ExecutorBase&) override {}
}
TheNoopExecution;

}

ExecutorBase::ExecutorBase() : m_executionContext(&TheNoopExecution) {}
ExecutorBase::ExecutorBase(ExecutionContext& context) : m_executionContext(&context) {}
ExecutorBase::~ExecutorBase() = default;

ExecutionContext& ExecutorBase::setExecutionContext(ExecutionContext& context)
{
    auto old = m_executionContext;
    m_executionContext = &context;
    return *old;
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

}
