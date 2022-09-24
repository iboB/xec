// Copyright (c) Borislav Stanimirov
// SPDX-License-Identifier: MIT
//
#pragma once
#include "API.h"

#include <chrono>
#include <memory>

namespace xec
{

class ExecutionContext;

class XEC_API ExecutorBase
{
public:
    ExecutorBase();
    ExecutorBase(std::shared_ptr<ExecutionContext> context);
    virtual ~ExecutorBase();

    virtual void update() = 0;

    // called before destruction to finalize all remaining execution
    virtual void finalize() {}

    // only applicable if no context has been set (either in the ctor or by calling this method)
    // until the context is set, a default one will be used and no execution will happen
    void setExecutionContext(std::shared_ptr<ExecutionContext> context);

    const std::shared_ptr<ExecutionContext>& executionContext() const { return m_executionContext; }

    void wakeUpNow();
    void scheduleNextWakeUp(std::chrono::milliseconds timeFromNow);
    void unscheduleNextWakeUp();
    void stop();
private:
    std::shared_ptr<ExecutionContext> m_executionContext; // never null

    // potentially points to a custom oneshot execution context which is used
    // when default constructed and then ignored throughout
    class InitialContext;
    InitialContext* m_initialContext = nullptr;
};

}
