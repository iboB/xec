// xec
// Copyright (c) 2020-2021 Borislav Stanimirov
//
// Distributed under the MIT Software License
// See accompanying file LICENSE.txt or copy at
// https://opensource.org/licenses/MIT
//
#include "TaskExecutor.hpp"

#include <cassert>
#include <atomic>
#include <algorithm>

namespace xec
{

TaskExecutor::TaskExecutor(std::chrono::milliseconds minTimeToSchedule)
    : m_minTimeToSchedule(minTimeToSchedule) {}

namespace
{
auto tnow() { return std::chrono::steady_clock::now(); }
}

struct TaskExecutor::TaskHasCToken
{
    TaskHasCToken(task_ctoken token) : m_token(token) {}
    bool operator()(const TaskWithId& task) const { return task.ctoken == m_token; }
    TaskExecutor::task_ctoken m_token;
};

TaskExecutor::task_id TaskExecutor::getNextTaskId()
{
    return m_freeTaskId++;
}

bool TaskExecutor::TimedTaskQueue::tryEraseId(task_id id)
{
    for (auto taskIter = c.begin(); taskIter != c.end(); ++taskIter)
    {
        if (taskIter->id == id)
        {
            c.erase(taskIter);
            std::make_heap(c.begin(), c.end(), comp);
            return true;
        }
    }
    return false;
}

size_t TaskExecutor::TimedTaskQueue::eraseTasksWithToken(task_ctoken token)
{
    auto newEnd = std::remove_if(c.begin(), c.end(), TaskHasCToken(token));
    auto size = c.end() - newEnd;
    if (size)
    {
        c.erase(newEnd, c.end());
        std::make_heap(c.begin(), c.end(), comp);
    }
    return size;
}

TaskExecutor::TimedTaskWithId TaskExecutor::TimedTaskQueue::topAndPop()
{
    std::pop_heap(c.begin(), c.end(), comp);
    auto ret = std::move(c.back());
    c.pop_back();
    return ret;
}

void TaskExecutor::update()
{
    m_tasksMutex.lock();
    std::vector<TaskWithId> tasks;
    tasks.swap(m_taskQueue);

    if (!m_timedTasks.empty())
    {
        const auto now = tnow();
        const auto maxTimeToExecute = now + m_minTimeToSchedule;
        while (true)
        {
            auto& top = m_timedTasks.top();
            if (top.time <= maxTimeToExecute)
            {
                auto& nt = tasks.emplace_back();
                nt.task = m_timedTasks.topAndPop().task;
                if (m_timedTasks.empty())
                {
                    unscheduleNextWakeUp();
                    break;
                }
            }
            else
            {
                const auto toWait = top.time - now;
                scheduleNextWakeUp(std::chrono::duration_cast<std::chrono::milliseconds>(toWait));
                break;
            }
        }
    }

    m_tasksMutex.unlock();

    for (auto& task : tasks)
    {
        task.task();
    }
}

void TaskExecutor::lockTasks()
{
    m_tasksMutex.lock();
    m_tasksLocked = true;
}

void TaskExecutor::unlockTasks()
{
    m_tasksLocked = false;
    m_tasksMutex.unlock();
    wakeUpNow(); // assuming something has changed
}

TaskExecutor::task_id TaskExecutor::pushTaskL(Task task, task_ctoken ownToken, task_ctoken tasksToCancelToken)
{
    assert(m_tasksLocked);
    cancelTasksWithTokenL(tasksToCancelToken);

    auto& newTask = m_taskQueue.emplace_back();
    newTask.task = std::move(task);
    newTask.id = getNextTaskId();
    newTask.ctoken = ownToken;
    return newTask.id;
}

TaskExecutor::task_id TaskExecutor::scheduleTaskL(std::chrono::milliseconds timeFromNow, Task task, task_ctoken ownToken, task_ctoken tasksToCancelToken)
{
    // no point in shceduling something which is about to happen so soon
    if (timeFromNow < m_minTimeToSchedule)
    {
        return pushTaskL(std::move(task));
    }

    assert(m_tasksLocked);
    cancelTasksWithTokenL(tasksToCancelToken);
    const auto newId = getNextTaskId();
    TimedTaskWithId newTask;
    newTask.task = std::move(task);
    newTask.id = newId;
    newTask.ctoken = ownToken;
    newTask.time = tnow() + timeFromNow;
    m_timedTasks.emplace(std::move(newTask));
    return newId;
}

bool TaskExecutor::cancelTask(task_id id)
{
    std::lock_guard<std::mutex> l(m_tasksMutex);
    return cancelTaskL(id);
}

bool TaskExecutor::cancelTaskL(task_id id)
{
    for (auto taskIter = m_taskQueue.cbegin(); taskIter != m_taskQueue.cend(); ++taskIter)
    {
        if (taskIter->id == id)
        {
            m_taskQueue.erase(taskIter);
            return true;
        }
    }

    return m_timedTasks.tryEraseId(id);
}

size_t TaskExecutor::cancelTasksWithToken(task_ctoken token)
{
    if (!token) return 0; // prevent lock on invalid token
    std::lock_guard<std::mutex> l(m_tasksMutex);
    return cancelTasksWithTokenL(token);
}

size_t TaskExecutor::cancelTasksWithTokenL(task_ctoken token)
{
    if (!token) return 0;
    auto newEnd = std::remove_if(m_taskQueue.begin(), m_taskQueue.end(), TaskHasCToken(token));
    auto size = m_taskQueue.end() - newEnd;
    m_taskQueue.erase(newEnd, m_taskQueue.end());
    return size + m_timedTasks.eraseTasksWithToken(token);
}

void TaskExecutor::finalize()
{
    if (m_finishTasksOnExit)
    {
        // since tasks can add other tasks, we need to loop mulitple times until we're done
        // we also intentionally ignore scheduled tasks in this context
        // (since they're not executed immediately they are not considered essential)
        while (true)
        {
            m_tasksMutex.lock();
            std::vector<TaskWithId> tasks;
            tasks.swap(m_taskQueue);
            m_tasksMutex.unlock();

            if (tasks.empty()) break;

            for (auto& task : tasks)
            {
                task.task();
            }
        }
    }
}

}
