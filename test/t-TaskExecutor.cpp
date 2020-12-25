// xec
// Copyright (c) 2020 Borislav Stanimirov
//
// Distributed under the MIT Software License
// See accompanying file LICENSE.txt or copy at
// https://opensource.org/licenses/MIT
//
#include <doctest/doctest.h>
#include <xec/TaskExecutor.hpp>
#include <xec/ThreadExecution.hpp>

#include <random>
#include <vector>
#include <functional>
#include <numeric>

TEST_SUITE_BEGIN("TaskExecutor");

struct TestThread;

class TaskExecutorExample : public xec::TaskExecutor
{
public:
    TaskExecutorExample()
    {
        m_testThread = std::make_unique<TestThread>(*this);
    }

    void incrementCounter()
    {
        ++m_counter;
    }

    void decrementCounter()
    {
        --m_counter;
    }

    void addToCounter(uint32_t adder)
    {
        m_counter += adder;
    }

    int counter() const
    {
        return m_counter;
    }

    TestThread* testThread() const
    {
        return m_testThread.get();
    }
private:
    int m_counter = 0;
    std::unique_ptr<TestThread> m_testThread;
};

struct TaskExecutorExampleTask
{
    TaskExecutorExampleTask(TaskExecutorExample& ee) : e(&ee) {};
    TaskExecutorExample* e;
};

struct IncrementCounterTask : TaskExecutorExampleTask
{
    using TaskExecutorExampleTask::TaskExecutorExampleTask;
    void operator()()
    {
        e->incrementCounter();
    }
};

struct DecrementCounterTask : TaskExecutorExampleTask
{
    using TaskExecutorExampleTask::TaskExecutorExampleTask;
    void operator()()
    {
        e->decrementCounter();
    }
};

struct AddIdToCounterTask : TaskExecutorExampleTask
{
    using TaskExecutorExampleTask::TaskExecutorExampleTask;
    std::shared_ptr<xec::TaskExecutor::task_id> id;
    void operator()()
    {
        e->addToCounter(uint32_t(*id));
    }
};

struct TestThread final : public xec::ExecutorBase
{
    explicit TestThread(TaskExecutorExample& executor)
        : m_executor(executor)
        , m_execution(*this)
    {
        m_execution.launchThread();
    }

    virtual ~TestThread() = default;

    virtual void update() override
    {
        m_executor.update();
        {
            std::lock_guard<std::mutex> l(m_finishedUpdateMutex);
            m_hasFinishedUpdate = true;
        }
        m_finishedUpdateCV.notify_one();
    }

    void waitForFinishedUpdate()
    {
        std::unique_lock<std::mutex> l(m_finishedUpdateMutex);
        m_finishedUpdateCV.wait(l, [this]() { return m_hasFinishedUpdate; });
        m_hasFinishedUpdate = false;
    }


    std::condition_variable m_finishedUpdateCV;
    std::mutex m_finishedUpdateMutex;
    bool m_hasFinishedUpdate = false;

    TaskExecutorExample& m_executor;
    xec::ThreadExecution m_execution;
};

static uint32_t generateRandomNumber()
{
    static std::random_device rd;
    static std::mt19937 mt(rd());
    static std::uniform_int_distribution<uint32_t> dist(1, 100);

    return dist(mt);
}

static std::vector<xec::TaskExecutor::task_id> pushIdTasks(TaskExecutorExample& executor, uint32_t taskCount)
{
    std::vector<xec::TaskExecutor::task_id> taskIds(taskCount);

    {
        auto taskLocker = executor.taskLocker();
        for (uint32_t i = 0; i < taskCount; ++i)
        {
            auto id = std::make_shared<xec::TaskExecutor::task_id>();
            AddIdToCounterTask task(executor);
            task.id = id;
            *id = taskLocker.pushTask(std::move(task));
            taskIds[i] = *id;
        }
    }

    return taskIds;
}


template <typename TaskType>
static void pushTasks(TaskExecutorExample& executor, uint32_t taskCount)
{
    auto taskLocker = executor.taskLocker();
    for (uint32_t i = 0; i < taskCount; ++i)
    {
        taskLocker.pushTask(TaskType(executor));
    }
}

TEST_CASE("pushTask")
{
    const auto taskCount = generateRandomNumber();
    TaskExecutorExample executor;

    executor.testThread()->waitForFinishedUpdate();

    REQUIRE(executor.counter() == 0);

    pushTasks<IncrementCounterTask>(executor, taskCount);

    executor.testThread()->wakeUpNow();
    executor.testThread()->waitForFinishedUpdate();

    REQUIRE(executor.counter() == taskCount);

    pushTasks<DecrementCounterTask>(executor, taskCount);
    pushTasks<IncrementCounterTask>(executor, 2*taskCount);
    pushTasks<DecrementCounterTask>(executor, taskCount);

    executor.testThread()->wakeUpNow();
    executor.testThread()->waitForFinishedUpdate();

    CHECK(executor.counter() == taskCount);
}

TEST_CASE("cancelTask")
{
    const auto taskCount = generateRandomNumber();
    TaskExecutorExample executor;

    executor.testThread()->waitForFinishedUpdate();

    auto taskIds = pushIdTasks(executor, taskCount);
    auto randTaskIndex = generateRandomNumber() % taskCount;
    REQUIRE(executor.cancelTask(taskIds[randTaskIndex]));

    executor.testThread()->wakeUpNow();
    executor.testThread()->waitForFinishedUpdate();

    auto expectedSum = std::accumulate(taskIds.begin(), taskIds.end(), uint64_t(0)) - taskIds[randTaskIndex];
    REQUIRE(executor.counter() == expectedSum);
}

class TestThreadFinish : public xec::TaskExecutor
{
public:
    explicit TestThreadFinish(bool execOnFinish)
        : m_execution(*this)
    {
        m_execution.launchThread();
        setFinishTasksOnExit(execOnFinish);
    }
    ~TestThreadFinish()
    {
        m_mutex.unlock();
    }
    std::mutex m_mutex;

    xec::ThreadExecution m_execution;
};

std::atomic_bool secondTaskFinished = false;


struct SleepTask {
    SleepTask(TestThreadFinish& tt) : t(tt) {}
    TestThreadFinish& t;
    void operator()() {
        t.pushTask([]() {
            secondTaskFinished = true;
        });
        std::lock_guard l(t.m_mutex);
    }
};

TEST_CASE("leaveTasks") {
    secondTaskFinished = false;
    {
        TestThreadFinish testThread(false);
        testThread.m_mutex.lock();
        testThread.pushTask(SleepTask(testThread));
    }
    REQUIRE(!secondTaskFinished);
}

TEST_CASE("finishTasks") {
    secondTaskFinished = false;
    {
        TestThreadFinish testThread(true);
        testThread.m_mutex.lock();
        testThread.pushTask(SleepTask(testThread));
    }
    REQUIRE(secondTaskFinished);
}
