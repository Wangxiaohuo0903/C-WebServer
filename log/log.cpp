#pragma once
#include "log.h"
#include <string.h>
#include <time.h>
#include <sys/time.h>
#include <stdarg.h>
#include <pthread.h>
using namespace std;

Log::Log() : m_count(0), m_is_async(false)
{
    m_buf = new char[m_log_buf_size];
    memset(m_buf, '\0', m_log_buf_size);
}

Log::~Log()
{
    if (write_thread && write_thread->joinable())
    {
        write_thread->join();
    }
    if (log_file)
    {
        log_file->close();
    }
}

bool Log::init(const char *file_name, int close_log, int log_buf_size, int split_lines, int max_queue_size)
{
    // 初始化日志队列
    if (max_queue_size >= 1)
    {
        m_is_async = true;
        m_log_queue = new block_queue<string>(max_queue_size);
        // 创建后台线程用于异步写入日志
        pthread_t tid;
        write_thread.reset(new std::thread(&Log::flush_log_thread, this));
    }

    // 设置日志参数
    m_close_log = close_log;
    m_log_buf_size = log_buf_size;
    m_buf = new char[m_log_buf_size];
    memset(m_buf, '\0', m_log_buf_size);
    m_split_lines = split_lines;

    // 获取当前日期信息，用于构造日志文件名
    time_t t = time(NULL);
    struct tm *sys_tm = localtime(&t);
    struct tm my_tm = *sys_tm;

    const char *p = strrchr(file_name, '/');
    char log_full_name[256] = {0};

    if (p == NULL)
    {
        // 如果文件名中不包含目录，则直接以日期为前缀
        snprintf(log_full_name, 255, "%d_%02d_%02d_%s", my_tm.tm_year + 1900, my_tm.tm_mon + 1, my_tm.tm_mday, file_name);
    }
    else
    {
        // 如果文件名中包含目录，则先提取目录和文件名，并以日期为前缀拼接成新的日志文件名
        strcpy(log_name, p + 1);
        strncpy(dir_name, file_name, p - file_name + 1);
        snprintf(log_full_name, 255, "%s%d_%02d_%02d_%s", dir_name, my_tm.tm_year + 1900, my_tm.tm_mon + 1, my_tm.tm_mday, log_name);
    }

    m_today = my_tm.tm_mday;

    // 打开日志文件
    log_file.reset(new std::ofstream());
    log_file->open(log_full_name, std::ios::app);
    if (!log_file->is_open())
    {
        return false;
    }

    return true;
}

void Log::write_log(int level, const char *format, ...)
{
    // 获取当前时间
    struct timeval now = {0, 0};
    gettimeofday(&now, NULL);
    time_t t = now.tv_sec;
    struct tm *sys_tm = localtime(&t);
    struct tm my_tm = *sys_tm;
    char s[16] = {0};

    // 根据日志级别选择相应的日志级别标识
    switch (level)
    {
    case 0:
        strcpy(s, "[debug]:");
        break;
    case 1:
        strcpy(s, "[info]:");
        break;
    case 2:
        strcpy(s, "[warn]:");
        break;
    case 3:
        strcpy(s, "[erro]:");
        break;
    default:
        strcpy(s, "[info]:");
        break;
    }

    // 对日志文件进行互斥访问，防止多线程同时写入导致数据错乱
    m_mutex.lock();
    m_count++;

    // 检查日志文件是否需要切分（按日期或文件大小）
    if (m_today != my_tm.tm_mday || m_count % m_split_lines == 0)
    {
        char new_log[256] = {0};
        char tail[16] = {0};
        snprintf(tail, 16, "%d_%02d_%02d_", my_tm.tm_year + 1900, my_tm.tm_mon + 1, my_tm.tm_mday);

        if (m_today != my_tm.tm_mday)
        {
            // 按日期切分日志文件
            snprintf(new_log, 255, "%s%s%s", dir_name, tail, log_name);
            m_today = my_tm.tm_mday;
            m_count = 0;
        }
        else
        {
            // 按文件大小切分日志文件
            snprintf(new_log, 255, "%s%s%s.%lld", dir_name, tail, log_name, m_count / m_split_lines);
        }
        log_file->close();
        log_file->open(new_log, std::ios::app);
    }

    m_mutex.unlock();

    // 使用可变参数列表处理日志格式化字符串
    va_list valst;
    va_start(valst, format);

    string log_str;
    m_mutex.lock();

    // 格式化日志字符串并存储在缓冲区m_buf中
    int n = snprintf(m_buf, 48, "%d-%02d-%02d %02d:%02d:%02d.%06ld %s ",
                     my_tm.tm_year + 1900, my_tm.tm_mon + 1, my_tm.tm_mday,
                     my_tm.tm_hour, my_tm.tm_min, my_tm.tm_sec, now.tv_usec, s);

    int m = vsnprintf(m_buf + n, m_log_buf_size - 1, format, valst);
    m_buf[n + m] = '\n';
    m_buf[n + m + 1] = '\0';
    log_str = m_buf;

    m_mutex.unlock();

    // 如果是异步写入日志模式且日志队列未满，则将日志字符串加入日志队列，以便后台线程异步写入
    if (m_is_async && !m_log_queue->full())
    {
        m_log_queue->push(log_str);
    }
    else
    {
        // 否则，同步写入日志文件
        m_mutex.lock();
        *log_file << log_str;
        m_mutex.unlock();
    }

    va_end(valst);
}

void Log::flush(void)
{
    m_mutex.lock();
    log_file->flush();
    m_mutex.unlock();
}
