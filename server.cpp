#pragma once
#include "server.h"

WebServer::WebServer() : m_thread_min_num(4), m_thread_max_num(1000), m_listenfd(-1), m_thread_pool(nullptr) {}

WebServer::~WebServer()
{
    close(m_listenfd);
    delete m_thread_pool;
}
void WebServer::init(int thread_num, int mode, int close_log)
{
    m_thread_min_num = 4;
    m_thread_max_num = thread_num;
    m_mode = static_cast<Mode>(mode);
    m_close_log = close_log;
}

void WebServer::thread_pool_init()
{
    m_thread_pool = new ThreadPool<HttpConn>(m_thread_min_num, m_thread_max_num);
}

void WebServer::log_init()
{
    Log::get_instance()->init("./ServerLog", m_close_log, 2000, 800000, 800);
}

void WebServer::event_listen()
{
    // 创建服务器套接字
    m_listenfd = socket(AF_INET, SOCK_STREAM, 0);

    if (m_listenfd == -1)
    {
        perror("socket failed");
        return;
    }
    // std::cout << "m_listenfd: " << m_listenfd << std::endl;
    //  设置服务器地址
    struct sockaddr_in server_addr;
    bzero(&server_addr, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(12346);
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);

    // 绑定服务器套接字
    int flag = 1;
    setsockopt(m_listenfd, SOL_SOCKET, SO_REUSEADDR, &flag, sizeof(flag));
    if (::bind(m_listenfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
    {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    // 开始监听
    if (listen(m_listenfd, 5) == -1)
    {
        perror("listen failed");
        exit(EXIT_FAILURE);
    }
}

void WebServer::event_loop()
{
    // 创建kqueue
    int kq = kqueue();

    // 创建事件
    struct kevent changes[1];
    EV_SET(&changes[0], m_listenfd, EVFILT_READ, EV_ADD, 0, 0, NULL);

    // 注册事件
    kevent(kq, changes, 1, NULL, 0, NULL);
    struct kevent events[10];
    while (true)
    {
        int nevents = kevent(kq, NULL, 0, events, 10, NULL);

        for (int i = 0; i < nevents; i++)
        {
            if (events[i].ident == m_listenfd)
            {
                // 新的连接
                struct sockaddr_in client_addr;
                socklen_t client_addrlen = sizeof(client_addr);
                int client_sock = accept(m_listenfd, (struct sockaddr *)&client_addr, &client_addrlen);
                int flags = fcntl(client_sock, F_GETFL, 0);
                if (flags == -1 || fcntl(client_sock, F_SETFL, flags | O_NONBLOCK) == -1)
                {
                    perror("fcntl failed");
                    close(client_sock);
                    continue;
                }
                EV_SET(&changes[0], client_sock, EVFILT_READ, EV_ADD, 0, 0, NULL);
                kevent(kq, changes, 1, NULL, 0, NULL);
            }
            else
            {
                // 已有的连接
                HttpConn *conn = HttpConnPool::getInstance()->acquire();
                conn->init(events[i].ident);
                conn->setMode(m_mode);

                // 在 Proactor 模式中，数据的读取应该在任务被添加到队列之前完成
                // 所以在这里我们需要先读取数据，然后再添加任务到队列
                if (m_mode == PROACTOR)
                {
                    conn->readData();
                    m_thread_pool->append(conn);
                }
                else if (m_mode == REACTOR)
                {
                    m_thread_pool->append(conn);
                }
            }
        }
    }
}
