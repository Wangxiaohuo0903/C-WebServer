#ifndef LOG_H
#define LOG_H

#include <memory>
#include <string>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <thread>
#include <fstream>
#include "block_queue.h"

enum class LogLevel
{
    INFO,
    WARNING,
    ERROR,
    FATAL
};

class Log
{
public:
    // 使用智能指针管理日志文件
    std::unique_ptr<std::ofstream> log_file;

    // 使用智能指针管理日志线程
    std::unique_ptr<std::thread> write_thread;

    static Log *get_instance()
    {
        static Log instance;
        return &instance;
    }

    static void *flush_log_thread(void *args)
    {
        Log::get_instance()->async_write_log();
    }

    int get_close_log() const
    {
        return m_close_log;
    }

    bool init(const char *file_name, int close_log, int log_buf_size = 8192, int split_lines = 5000000, int max_queue_size = 0);

    void write_log(int level, const char *format, ...);

    void flush(void);

private:
    Log();
    virtual ~Log();
    void *async_write_log()
    {
        std::string single_log;
        while (m_log_queue->pop(single_log))
        {
            if (log_file && log_file->is_open())
            {
                *log_file << single_log << std::endl;
            }
        }
    }

private:
    char dir_name[128];
    char log_name[128];
    int m_split_lines;
    int m_log_buf_size;
    long long m_count;
    int m_today;
    char *m_buf;
    block_queue<string> *m_log_queue;
    bool m_is_async;
    std::mutex m_mutex;
    int m_close_log;
};

#define LOG_DEBUG(format, ...)                                                                    \
    if (0 == Log::get_instance()->get_close_log())                                                \
    {                                                                                             \
        Log::get_instance()->write_log(static_cast<int>(LogLevel::DEBUG), format, ##__VA_ARGS__); \
        Log::get_instance()->flush();                                                             \
    }
#define LOG_INFO(format, ...)                                                                    \
    if (0 == Log::get_instance()->get_close_log())                                               \
    {                                                                                            \
        Log::get_instance()->write_log(static_cast<int>(LogLevel::INFO), format, ##__VA_ARGS__); \
        Log::get_instance()->flush();                                                            \
    }
#define LOG_WARN(format, ...)                                                                       \
    if (0 == Log::get_instance()->get_close_log())                                                  \
    {                                                                                               \
        Log::get_instance()->write_log(static_cast<int>(LogLevel::WARNING), format, ##__VA_ARGS__); \
        Log::get_instance()->flush();                                                               \
    }
#define LOG_ERROR(format, ...)                                                                    \
    if (0 == Log::get_instance()->get_close_log())                                                \
    {                                                                                             \
        Log::get_instance()->write_log(static_cast<int>(LogLevel::ERROR), format, ##__VA_ARGS__); \
        Log::get_instance()->flush();                                                             \
    }

#endif
