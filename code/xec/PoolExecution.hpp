// Copyright (c) Borislav Stanimirov
// SPDX-License-Identifier: MIT
//
#pragma once
#include <memory>

namespace xec {
class ExecutorBase;

class PoolExecution {
public:
    PoolExecution();
    ~PoolExecution();

    void addExecutor(ExecutorBase& executor);

public:
    class Impl;
    std::unique_ptr<Impl> m_impl;

    class PoolExecutionContext;
};

}
