// Copyright (c) Borislav Stanimirov
// SPDX-License-Identifier: MIT
//
#include "PoolExecution.hpp"
#include "ExecutorBase.hpp"
#include "ExecutionContext.hpp"

#include "ThreadName.hpp"

#include "impl/TimedQueue.hpp"
#include "impl/ordered_linear_set.hpp"

#include <itlib/qalgorithm.hpp>

#include <deque>
#include <thread>
#include <atomic>
#include <mutex>
#include <string>
#include <cassert>
#include <optional>
#include <unordered_set>
#include <condition_variable>

namespace xec {

class PoolExecution::Context final : public ExecutionContext {
    PoolExecution::Impl& m_execution;
    ExecutorBase& m_executor;
    std::atomic_bool m_running = true;

    // scheduled wake up time
    std::optional<clock_t::time_point> m_scheduledWakeUpTime;
public:
    Context(PoolExecution::Impl& execution, ExecutorBase& executor)
        : m_execution(execution)
        , m_executor(executor)
    {}

    ExecutorBase& executor() { return m_executor; }

    const std::optional<clock_t::time_point>& scheduledWakeUpTime() const noexcept { return m_scheduledWakeUpTime; }

    virtual void wakeUpNow() override;

    virtual void scheduleNextWakeUp(ms_t timeFromNow) override {
        m_scheduledWakeUpTime = clock_t::now() + timeFromNow;
    }

    virtual void unscheduleNextWakeUp() override {
        m_scheduledWakeUpTime.reset();
    }

    virtual void stop() override;

    virtual bool running() const override;
};

class PoolExecution::Impl {
public:
    // wait state
    std::condition_variable m_cv;
    std::mutex m_mutex;

    bool m_running = true; // running flag

    // scheduled wake up time
    std::optional<clock_t::time_point> m_scheduledWakeUpTime;

    ordered_linear_set<Context*> m_activeContexts; // currently being executed

    ordered_linear_set<Context*, std::deque> m_pendingContexts; // waiting to be executed

    std::unordered_set<Context*> m_allContexts; // all contexts

    struct TimedContext {
        Context* ctx;
        clock_t::time_point time;

        struct ByCtx {
            Context& ctx;
            bool operator()(const TimedContext& tc) const {
                return &ctx == tc.ctx;
            }
        };
    };

    TimedQueue<TimedContext> m_scheduledContexts;

    ~Impl() {
        // all contexts should be stopped before the pool is destroyed
        assert(m_allContexts.empty());
        assert(m_activeContexts.empty());
        assert(m_pendingContexts.empty());
    }

    void wakeUpNow(Context& ctx) {
        {
            std::lock_guard<std::mutex> lk(m_mutex);
            if (!m_pendingContexts.insert(&ctx)) {
                // an opportunity to prevent needless wakeups of workers
                // if the context is already pending, there's nothing to do and
                // * either workers are are waiting on a mutex lock to get it
                // * or they are sleeping because they can't get it, because it's already locked by another worker
                //   and this worker will get this context in its next iteration
                return;
            }

            // we added the context to the pending list, so remove it from the scheduled one
            m_scheduledContexts.eraseFirst(TimedContext::ByCtx{ ctx });
        }
        m_cv.notify_one();
    }

    Context* waitForContext(Context* contextToFree) {
        std::unique_lock<std::mutex> lock(m_mutex);

        if (contextToFree) {
            // the caller thread has released a context
            m_activeContexts.erase(contextToFree);

            auto& wakeupTime = contextToFree->scheduledWakeUpTime();
            if (wakeupTime) {
                if (m_pendingContexts.find(contextToFree) == m_pendingContexts.end()) {
                    // context is not pending wakeup, so we add it
                    m_scheduledContexts.push({ contextToFree, *wakeupTime });
                }
                // else this context is pending wakup anyway, so there's nothing to do
            }
            else {
                // this context doesn't have a scheduled wake up time
                // so we should remove it from the scheduled contexts (if it's there)
                m_scheduledContexts.eraseFirst(TimedContext::ByCtx{ *contextToFree });
            }
        }

        while (true) {
            if (!m_scheduledContexts.empty()) {
                // first, if we have scheduled contexts which are ready, move them to pending
                // and update the scheduled wake up time appropriately

                const auto now = clock_t::now();
                while (true) {
                    auto& top = m_scheduledContexts.top();

                    if (top.time <= now) {
                        m_pendingContexts.insert(top.ctx);
                        m_scheduledContexts.pop();
                        if (m_scheduledContexts.empty()) {
                            m_scheduledWakeUpTime.reset();
                            break;
                        }
                    }
                    else {
                        m_scheduledWakeUpTime = top.time;
                        break;
                    }
                }
            }

            for (auto i = m_pendingContexts.begin(); i != m_pendingContexts.end(); ++i) {
                auto ctx = *i;
                if (!m_activeContexts.insert(ctx)) {
                    // the context is already active (on another thread), so skip it
                    continue;
                }

                m_pendingContexts.erase(i);
                return *i;
            }

            if (!m_running) {
                // we have stopped runnign and there are no more pending contexts
                // this means they are all stopped and finalized and it's safe to tell the threads to stop
                // scheduled updates are just skipped (we assume they are not relevant enough)
                return nullptr;
            }

            if (m_scheduledWakeUpTime) {
                // wait until if we have a wake up time, wait for it
                auto status = m_cv.wait_until(lock, *m_scheduledWakeUpTime);

                if (status == std::cv_status::timeout) {
                    m_scheduledWakeUpTime.reset(); // timer was consumed
                }

                // otherwise we should... just do nothing
                // we either have pending contexts and will return them, or we don't and will loop again

                // this might be a spurious wake up
                // in such case we don't have work, and don't have a new timer
                // so we will loop again and wait on the same timer again
                // it may also be the case that we've many threads waited for the same wake up time and they all woke up
                // in this case we loop again and will wait indefinitely (or at the appropriate new wake up time)
            }
            else {
                // wait indefinitely
                // when we wake up we either have work and will return
                // or will have a scheduled wake up time and will wait for it
                // or it's a spurious wake up and will end up here again
                m_cv.wait(lock);
            }
        }

        return nullptr;
    }

    void run() {
        Context* ctx = nullptr;
        while (true) {
            ctx = waitForContext(ctx);

            if (!ctx) return;

            if (ctx->running()) {
                ctx->executor().update();
            }
            else {
                m_allContexts.erase(ctx);
                ctx->executor().finalize();
            }
        }
    }

    void stop() {
        {
            std::lock_guard<std::mutex> lk(m_mutex);
            if (!m_running) return; // already stopped
            m_running = false;
            for (auto ctx : m_allContexts) {
                ctx->stop();
            }
        }
        m_cv.notify_all();
    }

    void addExecutor(ExecutorBase& executor) {
        auto ctx = std::make_unique<Context>(*this, executor);
        m_allContexts.insert(ctx.get());
        executor.setExecutionContext(std::move(ctx));
    }

    std::vector<std::thread> m_threads;

    void launchThreads(size_t count, std::optional<std::string_view> threadName) {
        if (threadName) {
            for (size_t i = 0; i < count; ++i) {
                m_threads.emplace_back([this, name = std::string(*threadName) + std::to_string(i + 1)] {
                    SetThisThreadName(name);
                    run();
                });
            }
        }
        else {
            for (size_t i = 0; i < count; ++i) {
                m_threads.emplace_back([this] { run(); });
            }
        }
    }

    void joinThreads() {
        for (auto& t : m_threads) {
            t.join();
        }
    }

    void stopAndJoinThreads() {
        stop();
        joinThreads();
    }
};

void PoolExecution::Context::wakeUpNow() {
    if (running()) {
        m_execution.wakeUpNow(*this);
    }
}

void PoolExecution::Context::stop() {
    if (m_running.exchange(false, std::memory_order_release)) {
        // one final wake up se we can finalize the executor
        m_execution.wakeUpNow(*this);
    }
}

bool PoolExecution::Context::running() const {
    return m_running.load(std::memory_order_acquire);
}

PoolExecution::PoolExecution()
    : m_impl(new Impl)
{}
PoolExecution::~PoolExecution() = default;
void PoolExecution::addExecutor(ExecutorBase& executor) {
    m_impl->addExecutor(executor);
}
void PoolExecution::run() {
    m_impl->run();
}
void PoolExecution::stop() {
    m_impl->stop();
}
void PoolExecution::launchThreads(size_t count, std::optional<std::string_view> threadName) {
    m_impl->launchThreads(count, threadName);
}
void PoolExecution::joinThreads() {
    m_impl->joinThreads();
}
void PoolExecution::stopAndJoinThreads() {
    m_impl->stopAndJoinThreads();
}

}
