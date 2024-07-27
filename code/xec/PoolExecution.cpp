#include "PoolExecution.hpp"
#include "ExecutorBase.hpp"
#include "ExecutionContext.hpp"

#include <itlib/qalgorithm.hpp>

#include <list>
#include <thread>
#include <vector>
#include <cassert>
#include <optional>
#include <unordered_set>
#include <condition_variable>

namespace xec {

class PoolExecution::PoolExecutionContext : public ExecutionContext {
public:
    ExecutorBase* executor = nullptr;

    PoolExecution::Impl& m_execution;

    PoolExecutionContext(PoolExecution::Impl& execution)
        : m_execution(execution)
    {}

    virtual void wakeUpNow() override;

    virtual void scheduleNextWakeUp(ms_t timeFromNow) override;

    virtual void unscheduleNextWakeUp() override;

    virtual void stop() override;

    virtual bool running() const override;
};

class PoolExecution::Impl {
public:
    // wait state
    bool m_hasWork;
    std::condition_variable m_workCV;
    std::mutex m_workMutex;
    std::optional<clock_t::time_point> m_scheduledWakeUpTime;

    std::list<PoolExecutionContext> m_freeContexts; // pool to avoid allocations
    std::list<PoolExecutionContext> m_allocatedContexts; // contexts with executors

    std::unordered_set<PoolExecutionContext*> m_activeContexts; // use this to avoid duplicates
    std::vector<PoolExecutionContext*> m_orderedActiveContexts; // use this queue contexts in order of activation

    void wakeUpNow(PoolExecutionContext& ctx) {
        {
            std::lock_guard<std::mutex> lk(m_workMutex);

            auto inserted = m_activeContexts.insert(&ctx).second;

            if (!inserted) {
                // if the context is already active there's nothing to do
                // m_hasWork must be true (set by previous wakeups) which means that
                // * the context is already in the ordered list
                // * workers are are waiting on a mutex lock to get it
                assert(m_hasWork);
                return;
            }

            m_orderedActiveContexts.push_back(&ctx);
            m_hasWork = true;
        }
        m_workCV.notify_all();
    }

    void scheduleNextWakeUp(PoolExecutionContext& ctx, ms_t timeFromNow) {

    }
};


PoolExecution::PoolExecution(uint32_t numThreads)
    : m_impl(new Impl)
{}
PoolExecution::~PoolExecution() = default;

}
