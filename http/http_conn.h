// http_conn.h
#pragma once
#ifndef HTTP_CONN_H
#define HTTP_CONN_H

#include "http_request.h"
#include "../sql/sql_pool.h"

#include <list>
#include <mutex>

class HttpConn
{
public:
    HttpConn() : request() {}
    void init(int sockfd);
    void process();
    void handleRegister(const HttpRequest &request, HttpResponse &response);
    void handleLogin(const HttpRequest &request, HttpResponse &response);

private:
    HttpRequest request;
    int m_sockfd;
    static const int READ_BUFFER_SIZE = 1024;
    char m_read_buf[READ_BUFFER_SIZE];
    Log *m_logger;
    int m_close_log = 0;         // 是否开启日志
    SqlConnectionPool *sql_pool; //
};

class HttpConnPool
{
public:
    // 获取单例对象池实例
    static HttpConnPool *getInstance()
    {
        static HttpConnPool instance;
        return &instance;
    }

    // 从对象池中获取一个对象
    HttpConn *acquire()
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (m_pool.empty())
        {
            return new HttpConn();
        }
        else
        {
            HttpConn *conn = m_pool.front();
            m_pool.pop_front();
            return conn;
        }
    }

    // 将对象归还给对象池
    void release(HttpConn *conn)
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_pool.push_back(conn);
    }

private:
    HttpConnPool() {}
    std::list<HttpConn *> m_pool;
    std::mutex m_mutex;
};

#endif // HTTP_CONN_H
