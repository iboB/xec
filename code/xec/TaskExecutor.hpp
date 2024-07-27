// Copyright (c) Borislav Stanimirov
// SPDX-License-Identifier: MIT
//
#pragma once
#include "API.h"

#include "ExecutorBase.hpp"

#include <itlib/ufunction.hpp>

#include <mutex>
#include <vector>
#include <queue>

namespace xec {

class XEC_API TaskExecutor : public ExecutorBase {
public:
    // When scheduling tasks we use minTimeToSchedule to decide whether to schedule the task for later
    // or to execute it right away
    explicit TaskExecutor(ms_t minTimeToSchedule = ms_t(20));

    virtual void update() override;
    virtual void finalize() override;

    void setFinishTasksOnExit(bool b) { m_finishTasksOnExit = b; }

    // tasks
    // tasks are pushed from various threads
    // tasks are executed on update
    using Task = itlib::ufunction<void()>;
    using task_id = uint32_t;
    using task_ctoken = uint32_t; // cancellation token

    // locker raii interface
    class TaskLocker {
    public:
        explicit TaskLocker(TaskExecutor* e) : m_executor(e) {
            e->lockTasks();
        }
        TaskLocker(const TaskLocker&) = delete;
        TaskLocker& operator=(const TaskLocker&) = delete;
        TaskLocker(TaskLocker&& other) noexcept : m_executor(other.m_executor) {
            other.m_executor = nullptr;
        }
        TaskLocker& operator=(TaskLocker&& other) noexcept = delete;
        ~TaskLocker() {
            if (m_executor) m_executor->unlockTasks();
        }
        task_id pushTask(Task task, task_ctoken ownToken = 0, task_ctoken tasksToCancelToken = 0) {
            return m_executor->pushTaskL(std::move(task), ownToken, tasksToCancelToken);
        }
        task_id scheduleTask(ms_t timeFromNow, Task task, task_ctoken ownToken = 0, task_ctoken tasksToCancelToken = 0) {
            return m_executor->scheduleTaskL(timeFromNow, std::move(task), ownToken, tasksToCancelToken);
        }
        bool rescheduleTask(ms_t timeFromNow, task_id id) {
            return m_executor->rescheduleTaskL(timeFromNow, id);
        }
    private:
        TaskExecutor* m_executor;
    };
    TaskLocker taskLocker() { return TaskLocker(this); }

    task_id pushTask(Task task, task_ctoken ownToken = 0, task_ctoken tasksToCancelToken = 0) {
        return taskLocker().pushTask(std::move(task), ownToken, tasksToCancelToken);
    }

    task_id scheduleTask(ms_t timeFromNow, Task task, task_ctoken ownToken = 0, task_ctoken tasksToCancelToken = 0) {
        return taskLocker().scheduleTask(timeFromNow, std::move(task), ownToken, tasksToCancelToken);
    }

    bool rescheduleTask(ms_t timeFromNow, task_id id) {
        return taskLocker().rescheduleTask(timeFromNow, id);
    }

    // task locking
    // you need to lock the tasks with these functions or a locker before adding tasks
    void lockTasks();
    void unlockTasks();

    // only valid on any thread when tasks are locked
    task_id pushTaskL(Task task, task_ctoken ownToken = 0, task_ctoken tasksToCancelToken = 0);
    task_id scheduleTaskL(ms_t timeFromNow, Task task, task_ctoken ownToken = 0, task_ctoken tasksToCancelToken = 0);

    // cancel the task successfully and return true if the task queue containing the task hasn't started executing.
    // return whether the task was removed from the pending tasks
    // WARNING: if this returns false, one of three things might be true:
    // * The task was never added (bad id)
    // * The task is currently executing
    // * The task has finished executing
    bool cancelTask(task_id id);
    bool cancelTaskL(task_id id); // only valid on any thread when tasks are locked

    // reschedule a scheduled task
    // return true if the reschedule was successful
    // the conditions to return false are the same as the ones from cancelTask
    bool rescheduleTaskL(ms_t timeFromNow, task_id id);

    // cancel tasks which were added with a given token and return the number successfully cancelled
    // WARNING: tasks which are currently executing won't be cancelled; the number of such tasks may be more than one!
    size_t cancelTasksWithToken(task_ctoken token);
    size_t cancelTasksWithTokenL(task_ctoken token); // only valid on any thread when tasks are locked
private:
    const ms_t m_minTimeToSchedule;

    bool m_tasksLocked = false;  // a silly defence but should work most of the time
    bool m_finishTasksOnExit = false;
    std::mutex m_tasksMutex;

    task_id m_freeTaskId = 0;
    task_id getNextTaskIdL();

    struct TaskWithId {
        Task task;
        task_id id;
        task_ctoken ctoken;
    };
    std::vector<TaskWithId> m_taskQueue;

    // the access to this vector are strictly ordered
    // it's only touched in update and finalize
    // it's serves as a double-buffer for tasks from the queue
    // it's purpose is to save allocations for adding new tasks
    // instead, eventually this vector and the tasks queue vector will reach a peak capacity
    // and new tasks won't lead to allocations
    std::vector<TaskWithId> m_executingTasks;
    void fillExecutingTasksL();
    void executeTasks();

    struct TimedTaskWithId : public TaskWithId {
        clock_t::time_point time;
        struct Later {
            bool operator()(const TimedTaskWithId& a, const TimedTaskWithId& b) {
                return a.time > b.time;
            }
        };
    };

    // adapt more so we can erase tasks
    struct TimedTaskQueue
        : public std::priority_queue<TimedTaskWithId, std::vector<TimedTaskWithId>, TimedTaskWithId::Later>
    {
        TaskWithId tryExtractId(task_id id); // will return empty task if not successful
        bool tryRescheduleId(clock_t::time_point newTime, task_id id);
        size_t eraseTasksWithToken(task_ctoken token);
        TimedTaskWithId topAndPop();
        void clear() { c.clear(); }
    };
    TimedTaskQueue m_timedTasks;

    struct TaskHasCToken; // helper for searches by token
};

}
