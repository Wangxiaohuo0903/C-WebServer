// http_conn.h
#ifndef HTTP_CONN_H
#define HTTP_CONN_H

#include "http_request.h"

class HttpConn
{
public:
    HttpConn() : request() {}
    void init(int sockfd);
    void process();

private:
    HttpRequest request;
    int m_sockfd;
    static const int READ_BUFFER_SIZE = 1024;
    char m_read_buf[READ_BUFFER_SIZE];
    Log *m_logger;
    int m_close_log = 0; // 是否开启日志
};

#endif // HTTP_CONN_H
