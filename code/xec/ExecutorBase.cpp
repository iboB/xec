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

namespace xec
{

ExecutorBase::ExecutorBase() = default;
ExecutorBase::ExecutorBase(ExecutionContext& context) : m_executionContext(&context) {}
ExecutorBase::~ExecutorBase() = default;

void ExecutorBase::setExecutionContext(ExecutionContext& context)
{
    assert(!m_executionContext);
    m_executionContext = &context;
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
