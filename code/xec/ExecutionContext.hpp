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

class ExecutorBase;

class XEC_API ExecutionContext
{
public:
    virtual ~ExecutionContext(); // defined in ExecutorBase.cpp, not here but in a cpp so as to export the vtable

    // signal that the executor needs to be updated
    virtual void wakeUpNow(ExecutorBase& e) = 0;

    // schedule a wake up
    // NOTE that if a wake up happens before the scheduled time (by wakeUpNow) the scheduled time is forgotten
    // NOTE that if two calls to schedule a wake up happen before a wake up, the second one will override the first
    virtual void scheduleNextWakeUp(ExecutorBase& e, std::chrono::milliseconds timeFromNow) = 0;
    virtual void unscheduleNextWakeUp(ExecutorBase& e) = 0;

    // called by an executor when it determines that it wants to be stopped
    virtual void stop(ExecutorBase& e) = 0;
};

}
