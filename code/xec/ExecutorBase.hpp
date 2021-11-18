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
#include <type_traits>

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

    // hacky pimpl
    // this actually stores a custom oneshot execution context which is used at the beginning
    // and then ignored throughout
    class InitialContext;
    using Buf = std::aligned_storage_t<3 * sizeof(size_t) + sizeof(std::chrono::steady_clock::time_point), alignof(void*)>;
    Buf m_initialContextBuffer;
    InitialContext* m_initialContext = nullptr;
};

}
