#include "server.h"

WebServer::WebServer() : m_thread_num(0), m_listenfd(-1), m_thread_pool(nullptr) {}

WebServer::~WebServer()
{
    close(m_listenfd);
    delete m_thread_pool;
}
void WebServer::init(int thread_num)
{
    m_thread_num = thread_num;
}

void WebServer::thread_pool_init()
{
    m_thread_pool = new ThreadPool(m_thread_num);
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
    // 设置服务器地址
    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(12345);
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);

    // 绑定服务器套接字
    if (bind(m_listenfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1)
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

    while (true)
    {
        struct kevent events[10];
        int nevents = kevent(kq, NULL, 0, events, 10, NULL);

        struct sockaddr_in client_addr;
        socklen_t client_addrlen = sizeof(client_addr);
        for (int i = 0; i < nevents; i++)
        {
            if (events[i].ident == m_listenfd)
            {
                // 新的连接
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
                http_conn *conn = new http_conn;
                conn->init(events[i].ident);
                m_thread_pool->enqueue([conn]
                                       { conn->process(); delete conn; });
            }
        }
    }
}