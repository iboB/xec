// Copyright (c) Borislav Stanimirov
// SPDX-License-Identifier: MIT
//
#pragma once
#include "ExecutionContext.hpp"

#include <memory>
#include <atomic>

#include <vector>

namespace xec {

class XEC_API PoolExecutionContext : public ExecutionContext {
public:
private:
};

class XEC_API PoolExecution {
public:
    PoolExecution(uint32_t numThreads);
    ~PoolExecution();

    void launchThreads();
    void joinThreads();
public:
    std::vector<std::shared_ptr<PoolExecutionContext>> m_contexts;
};

}
