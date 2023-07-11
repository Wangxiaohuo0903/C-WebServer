#include "http_conn.cpp"
#include "threadpool.h"
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/event.h>
#include <sys/time.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <fcntl.h>
class WebServer
{
public:
    WebServer();
    ~WebServer();
    void init(int thread_num);
    void thread_pool_init();
    void sql_pool();
    void log_write();
    void event_listen();
    void event_loop();

public:
    int m_thread_num;
    int m_listenfd;
    ThreadPool *m_thread_pool;
};