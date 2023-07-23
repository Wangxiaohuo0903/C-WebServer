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

template <typename T>
class ConcurrentQueue
{
public:
    ConcurrentQueue() : m_queue(), m_mutex(), m_condition(), m_stopped(false) {}

    ~ConcurrentQueue() {}

    void push(T value)
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_queue.push(value);
        m_condition.notify_one();
    }

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

    void stop()
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_stopped = true;
        m_condition.notify_all();
    }

    bool empty() const
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_queue.empty();
    }

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
        for (std::thread &worker : m_workers)
            worker.join();
    }

    // reactor模式下的请求入队
    bool append(T *request)
    {
        std::unique_lock<std::mutex> lock(m_mutex);
        if (m_tasks.size() >= m_maxThreads)
        {
            return false;
        }
        // 读写事件
        // request->m_state = state;
        m_tasks.push_back(request);
        m_condition.notify_one();
        return true;
    }

    // proactor模式下的请求入队
    bool append_p(T *request)
    {
        std::unique_lock<std::mutex> lock(m_mutex);
        if (m_tasks.size() >= m_maxThreads)
        {
            return false;
        }
        m_tasks.push_back(request);
        m_condition.notify_one();
        return true;
    }

private:
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
            task->process();
        }
    }

    std::list<T *> m_tasks;
    std::list<std::thread> m_workers;
    std::mutex m_mutex;
    std::condition_variable m_condition;
    bool m_stop;
    size_t m_minThreads;
    size_t m_maxThreads;
};
#endif
