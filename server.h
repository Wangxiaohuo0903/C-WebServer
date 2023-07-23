#pragma once
#include "http/http_conn.h"
#include "threadpool.h"
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/event.h>
#include <sys/time.h>
#include <unistd.h>
#include <fcntl.h>
#include "log/log.h"

class WebServer
{
public:
    WebServer();
    ~WebServer();
    void init(int thread_num, int mode, int close_log);
    void thread_pool_init();
    void sql_pool();
    void log_init();
    void event_listen();
    void event_loop();

public:
    int m_thread_num;
    int m_requests_num;
    int m_listenfd;
    int m_close_log;
    Mode m_mode;
    ThreadPool<HttpConn> *m_thread_pool;
};
