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

class ThreadPool
{
public:
    ThreadPool(size_t threads) : workers(threads)
    {
        for (auto &worker : workers)
        {
            worker = std::thread([this]
                                 { this->worker(); });
        }
    }
    ~ThreadPool()
    {
        tasks.stop();
        for (auto &worker : workers)
        {
            if (worker.joinable())
            {
                worker.join();
            }
        }
    }

    template <class F, class... Args>
    auto enqueue(F &&f, Args &&...args) -> std::future<typename std::result_of<F(Args...)>::type>
    {
        using return_type = typename std::result_of<F(Args...)>::type;

        auto task = std::make_shared<std::packaged_task<return_type()>>(std::bind(std::forward<F>(f), std::forward<Args>(args)...));

        std::future<return_type> res = task->get_future();
        tasks.push([task]()
                   { (*task)(); });

        return res;
    }

private:
    // 线程池
    std::vector<std::thread> workers;
    // 任务队列
    ConcurrentQueue<std::function<void()>> tasks;

    void worker()
    {
        while (true)
        {
            std::function<void()> task;
            if (!tasks.pop(task))
            {
                break;
            }
            task();
        }
    }
};

#endif
