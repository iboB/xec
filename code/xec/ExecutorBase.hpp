// Copyright (c) Borislav Stanimirov
// SPDX-License-Identifier: MIT
//
#pragma once
#include "API.h"
#include "impl/chrono.hpp"
#include <memory>

namespace xec {

class ExecutionContext;

class XEC_API ExecutorBase {
public:
    ExecutorBase();
    ExecutorBase(ExecutionContext& context);
    virtual ~ExecutorBase();

    virtual void update() = 0;

    // called before destruction to finalize all remaining execution
    virtual void finalize() {}

    // only applicable if no context has been set (either in the ctor or by calling this method)
    // until the context is set, a default one will be used and no execution will happen
    void setExecutionContext(ExecutionContext& context);

    const ExecutionContext& executionContext() const { return *m_executionContext; }

    // proxies to the execution context
    void wakeUpNow();
    void scheduleNextWakeUp(ms_t timeFromNow);
    void unscheduleNextWakeUp();
    void stop();
private:
    // potentially points to a custom oneshot execution context which is used
    // when default constructed and then ignored throughout
    class InitialContext;
    std::unique_ptr<InitialContext> m_initialContext;

    ExecutionContext* m_executionContext; // never null
};

}
