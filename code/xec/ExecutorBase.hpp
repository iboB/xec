// xec
// Copyright (c) 2020-2021 Borislav Stanimirov
//
// Distributed under the MIT Software License
// See accompanying file LICENSE.txt or copy at
// https://opensource.org/licenses/MIT
//
#pragma once

#include "API.h"
#include <chrono>

namespace xec
{

class ExecutionContext;

class XEC_API ExecutorBase
{
public:
    ExecutorBase();
    ExecutorBase(ExecutionContext& context);
    virtual ~ExecutorBase();

    virtual void update() = 0;

    // called before destruction to finalize all remaining execution
    virtual void finalize() {}

    // only applicable if no context has been set (either in the ctor or by calling this method)
    // no executor operations are valid until an execution context has been set
    void setExecutionContext(ExecutionContext& context);

    ExecutionContext& executionContext() const { return *m_executionContext; }

    void wakeUpNow();
    void scheduleNextWakeUp(std::chrono::milliseconds timeFromNow);
    void unscheduleNextWakeUp();
    void stop();
private:
    ExecutionContext* m_executionContext = nullptr;
};

}
