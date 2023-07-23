#pragma once
#ifndef THREADPOOL_H
#define THREADPOOL_H

#include <vector>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <atomic>
#include <future>
#include <list> // 添加 list 头文件以使用 std::list

template <typename T>
class ConcurrentQueue
{
public:
    ConcurrentQueue() : m_queue(), m_mutex(), m_condition(), m_stopped(false) {}

    ~ConcurrentQueue() {}

    // 将一个值放入队列
    void push(T value)
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_queue.push(value);
        m_condition.notify_one();
    }

    // 从队列中取出一个值
    bool pop(T &value)
    {
        std::unique_lock<std::mutex> lock(m_mutex);
        m_condition.wait(lock, [this]
                         { return !m_queue.empty() || m_stopped; });
        if (m_stopped && m_queue.empty())
        {
            return false;
        }
        value = m_queue.front();
        m_queue.pop();
        return true;
    }

    // 停止队列（用于停止工作线程）
    void stop()
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_stopped = true;
        m_condition.notify_all();
    }

    // 检查队列是否为空
    bool empty() const
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_queue.empty();
    }

    // 获取队列大小
    size_t size() const
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_queue.size();
    }

private:
    std::queue<T> m_queue;
    mutable std::mutex m_mutex;
    std::condition_variable m_condition;
    std::atomic<bool> m_stopped;
};

template <typename T>
class ThreadPool
{
public:
    ThreadPool(size_t minThreads, size_t maxThreads)
        : m_minThreads(minThreads), m_maxThreads(maxThreads), m_stop(false)
    {
        for (size_t i = 0; i < m_minThreads; ++i)
        {
            // 创建线程并将其添加到工作线程列表
            m_workers.emplace_back(std::thread(&ThreadPool::workerThread, this));
        }
    }

    ~ThreadPool()
    {
        {
            std::unique_lock<std::mutex> lock(m_mutex);
            m_stop = true;
        }
        m_condition.notify_all();
        // 等待所有工作线程退出
        for (std::thread &worker : m_workers)
            worker.join();
    }

    // Reactor 模式：将一个请求添加到队列
    bool append(T *request)
    {
        std::unique_lock<std::mutex> lock(m_mutex);
        if (m_tasks.size() >= m_maxThreads)
        {
            return false;
        }
        // 将请求加入队列中
        m_tasks.push_back(request);
        m_condition.notify_one();
        return true;
    }

private:
    // 工作线程的函数，用于处理队列中的任务
    void workerThread()
    {
        while (true)
        {
            T *task;
            {
                std::unique_lock<std::mutex> lock(m_mutex);
                m_condition.wait(lock, [this]
                                 { return this->m_stop || !this->m_tasks.empty(); });
                if (this->m_stop && this->m_tasks.empty())
                    return;
                task = m_tasks.front();
                m_tasks.pop_front();
            }
            // 处理任务
            task->process();
        }
    }

    std::list<T *> m_tasks;           // 使用 list 维护任务队列，以保持任务的顺序
    std::list<std::thread> m_workers; // 使用 list 维护工作线程的顺序
    std::mutex m_mutex;
    std::condition_variable m_condition;
    bool m_stop;
    size_t m_minThreads;
    size_t m_maxThreads;
};
#endif
