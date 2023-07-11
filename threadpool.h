#ifndef THREADPOOL_H
#define THREADPOOL_H

#include <vector>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <functional>

class ThreadPool
{
public:
    ThreadPool(size_t threads);
    ~ThreadPool();

    template <class F, class... Args>
    void enqueue(F &&f, Args &&...args);

private:
    // 线程池
    std::vector<std::thread> workers;
    // 任务队列
    std::queue<std::function<void()>> tasks;

    // 同步
    std::mutex queue_mutex;
    std::condition_variable condition;
    bool stop;
};

// 构造函数
inline ThreadPool::ThreadPool(size_t threads)
    : stop(false)
{
    for (size_t i = 0; i < threads; ++i)
        workers.emplace_back(
            [this]
            {
                for (;;)
                {
                    std::function<void()> task;

                    {
                        std::unique_lock<std::mutex> lock(this->queue_mutex);
                        this->condition.wait(lock,
                                             [this]
                                             { return this->stop || !this->tasks.empty(); });
                        if (this->stop && this->tasks.empty())
                            return;
                        task = std::move(this->tasks.front());
                        this->tasks.pop();
                    }

                    task();
                }
            });
}

// 添加新的工作任务
template <class F, class... Args>
void ThreadPool::enqueue(F &&f, Args &&...args)
{
    {
        std::unique_lock<std::mutex> lock(queue_mutex);

        // 添加任务到任务队列
        tasks.emplace(std::bind(std::forward<F>(f), std::forward<Args>(args)...));
    }
    condition.notify_one();
}

// 析构函数
inline ThreadPool::~ThreadPool()
{
    {
        std::unique_lock<std::mutex> lock(queue_mutex);
        stop = true;
    }
    condition.notify_all();
    for (std::thread &worker : workers)
        worker.join();
}

#endif
