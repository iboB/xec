// xec
// Copyright (c) 2020-2021 Borislav Stanimirov
//
// Distributed under the MIT Software License
// See accompanying file LICENSE.txt or copy at
// https://opensource.org/licenses/MIT
//
#pragma once
#include "NoopExecution.hpp"

namespace xec
{
namespace
{
class NoopExecutionContextImpl final : public ExecutionContext
{
    virtual void wakeUpNow(xec::ExecutorBase&) override {}
    virtual void stop(xec::ExecutorBase&) override {}
    virtual void scheduleNextWakeUp(xec::ExecutorBase&, std::chrono::milliseconds) override {}
    virtual void unscheduleNextWakeUp(xec::ExecutorBase&) override {}
};
}

ExecutionContext& NoopExecutionContext()
{
    static NoopExecutionContextImpl ctx;
    return ctx;
}

}
