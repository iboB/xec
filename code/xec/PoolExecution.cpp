// Copyright (c) Borislav Stanimirov
// SPDX-License-Identifier: MIT
//
#include "PoolExecution.hpp"
#include "ExecutorBase.hpp"
#include "ExecutionContext.hpp"

#include <itlib/qalgorithm.hpp>

#include <deque>
#include <thread>
#include <vector>
#include <cassert>
#include <optional>
#include <unordered_set>
#include <condition_variable>

namespace xec {

class PoolExecution::PoolExecutionContext : public ExecutionContext {
    ExecutorBase* m_executor = nullptr;
    bool m_running = true;
    PoolExecution::Impl& m_execution;
public:
    PoolExecutionContext(PoolExecution::Impl& execution)
        : m_execution(execution)
    {}

    virtual void wakeUpNow() override;

    virtual void scheduleNextWakeUp(ms_t timeFromNow) override;

    virtual void unscheduleNextWakeUp() override;

    virtual void stop() override;

    virtual bool running() const override;
};

template <typename T, template <typename ...> typename Container = std::vector>
class ordered_linear_set : private Container<T> {
public:
    using container = Container<T>;
    using iterator = typename container::iterator;

    container& c() { return *this; }
    const container& c() const { return *this; }

    bool insert(T&& t) {
        for (const auto& e : c()) {
            if (e == t) return false;
        }
        c().push_back(std::forward<T>(t));
        return true;
    }

    iterator find(const T& t) {
        auto i = begin();
        for (; i != end(); ++i) {
            if (*i == t) break;
        }
        return i;
    }

    using container::begin;
    using container::end;


    using container::erase;
    bool erase(const T& t) {
        auto i = find(t);
        if (i == end()) return false;
        erase(i);
        return true;
    }

private:
};

class PoolExecution::Impl {
public:
    // wait state
    bool m_hasWork;
    std::condition_variable m_workCV;
    std::mutex m_workMutex;

    std::deque<PoolExecutionContext> m_contexts; // sparse array (by PoolExecutionContext::executor)

    ordered_linear_set<PoolExecutionContext*> m_activeContexts; // currently being executed

    ordered_linear_set<PoolExecutionContext*, std::deque> m_pendingContexts; // waiting to be executed

    void wakeUpNow(PoolExecutionContext& ctx) {
        {
            std::lock_guard<std::mutex> lk(m_workMutex);
            if (!m_pendingContexts.insert(&ctx)) {
                // an opportunity to prevent needless wakeups of workers
                // if the context is already pending, there's nothing to do
                // m_hasWork must be true (set by previous wakeups) which means that
                // * either workers are are waiting on a mutex lock to get it
                // * or they are sleeping because they can't get it, because it's already locked by another worker
                //   and this worker will get this context in its next iteration
                assert(m_hasWork);
                return;
            }
            m_hasWork = true;
        }
        m_workCV.notify_all();
    }

    void scheduleNextWakeUp(PoolExecutionContext& ctx, ms_t timeFromNow) {

    }

    void unscheduleNextWakeUp(PoolExecutionContext& ctx) {

    }

    void stop(PoolExecutionContext& ctx) {
    }
};


PoolExecution::PoolExecution(uint32_t numThreads)
    : m_impl(new Impl)
{}
PoolExecution::~PoolExecution() = default;

}
