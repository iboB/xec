// xec
// Copyright (c) 2020-2021 Borislav Stanimirov
//
// Distributed under the MIT Software License
// See accompanying file LICENSE.txt or copy at
// https://opensource.org/licenses/MIT
//
#include <doctest/doctest.h>
#include <xec/TaskExecutor.hpp>
#include <xec/ThreadExecution.hpp>

#include <atomic>

TEST_SUITE_BEGIN("TaskScheduling");

class Executor : public xec::TaskExecutor
{
public:
    Executor() : e(*this) {
        e.launchThread();
    }

    virtual void update() override
    {
        xec::TaskExecutor::update();
        ++numUpdates;
    }

    int numUpdates = 0;
    xec::ThreadExecution e;
};

struct TaskStatus
{
    int32_t t1 = 0;
    int32_t t2 = 0;
    std::atomic_int32_t t3 = 0;
};

struct Task
{
    Task(TaskStatus& s) : status(s) {}
    TaskStatus& status;
};

struct Task1 : public Task
{
    using Task::Task;
    void operator()()
    {
        ++status.t1;
    }
};

struct Task2 : public Task
{
    using Task::Task;
    void operator()()
    {
        ++status.t2;
    }
};

struct Task3 : public Task
{
    using Task::Task;
    void operator()()
    {
        ++status.t3;
    }
};

TEST_CASE("Too late")
{
    Executor xec;
    TaskStatus status;
    xec.setFinishTasksOnExit(true);
    {
        auto t = xec.taskLocker();
        t.pushTask(Task1(status));
        t.scheduleTask(std::chrono::milliseconds(1), Task2(status));
        t.scheduleTask(std::chrono::seconds(100), Task3(status));
    }
    xec.e.stopAndJoinThread();

    CHECK(status.t1 == 1);
    CHECK(status.t2 == 1);
    CHECK(status.t3 == 0);
}

TEST_CASE("Execute")
{
    Executor xec;
    TaskStatus status;
    xec.setFinishTasksOnExit(true);
    {
        auto t = xec.taskLocker();
        t.pushTask(Task1(status));
        t.scheduleTask(std::chrono::milliseconds(50), Task3(status));
        t.scheduleTask(std::chrono::milliseconds(40), [&status] {
            CHECK(status.t1 == 1);
            CHECK(status.t2 == 1);
            CHECK(status.t3 == 0);
            ++status.t2;
        });
        t.scheduleTask(std::chrono::milliseconds(30), Task2(status));
    }
    while(status.t3 == 0) std::this_thread::yield();
    xec.e.stopAndJoinThread();

    CHECK(status.t1 == 1);
    CHECK(status.t2 == 2);
    CHECK(status.t3 == 1);
}

TEST_CASE("reschedule")
{
    Executor xec;
    TaskStatus status;
    xec.setFinishTasksOnExit(true);

    {
        auto t = xec.taskLocker();
        auto id = t.scheduleTask(std::chrono::milliseconds(30), Task3(status));
        t.scheduleTask(std::chrono::milliseconds(50), Task2(status));

        CHECK(t.rescheduleTask(std::chrono::milliseconds(60), id));
    }

    while(status.t3 == 0) std::this_thread::yield();
    xec.e.stopAndJoinThread();

    CHECK(status.t2 == 1);
}
