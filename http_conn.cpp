#include "lock.h"
#include <unistd.h>
#include <string.h>

enum class ParseState
{
    REQUEST_LINE,
    HEADERS,
    BODY,
    DONE,
};

enum class LineState
{
    LINE_OK,
    LINE_BAD,
    LINE_OPEN,
};

class http_conn
{
public:
    http_conn() {}
    ~http_conn() {}

    void init(int sockfd)
    {
        m_sockfd = sockfd;
        memset(m_read_buf, 0, READ_BUFFER_SIZE);
    }

    void process()
    {
        // 读取请求
        int n = read(m_sockfd, m_read_buf, READ_BUFFER_SIZE - 1);
        if (n <= 0)
        {
            // 出错或连接关闭
            close(m_sockfd);
            return;
        }

        // 解析请求（这里我们只处理GET请求）
        if (strncmp(m_read_buf, "GET", 3) != 0)
        {
            // 不是GET请求，返回400 Bad Request
            const char *response = "HTTP/1.1 400 Bad Request\r\n\r\n";
            write(m_sockfd, response, strlen(response));
            close(m_sockfd);
            return;
        }

        // 生成响应
        const char *response = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\nHello, World!";
        write(m_sockfd, response, strlen(response));

        // 关闭连接
        close(m_sockfd);
    }

private:
    int m_sockfd;
    static const int READ_BUFFER_SIZE = 1024;
    char m_read_buf[READ_BUFFER_SIZE];
};
