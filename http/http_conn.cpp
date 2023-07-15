#pragma once

#include "http_conn.h"
enum HTTP_CODE
{
    NO_REQUEST,
    GET_REQUEST,
    BAD_REQUEST,
    NO_RESOURCE,
    FORBIDDEN_REQUEST,
    FILE_REQUEST,
    INTERNAL_ERROR,
    CLOSED_CONNECTION
};
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

void HttpConn::init(int sockfd)
{
    m_sockfd = sockfd;
    // memset(m_read_buf, 0, READ_BUFFER_SIZE);
}

void HttpConn::process()
{
    // 读取请求
    HttpResponse response;
    int n = read(m_sockfd, m_read_buf, READ_BUFFER_SIZE - 1);
    if (n <= 0)
    {
        // 出错或连接关闭
        close(m_sockfd);
        return;
    }

    // 解析请求（这里我们只处理GET请求）
    // if (strncmp(m_read_buf, "GET", 3) != 0)
    if (!request.parse(m_read_buf))
    {
        // 不是GET请求，返回400 Bad Request
        response = HttpResponse::makeErrorResponse(400);
    }

    // 生成响应

    if (request.method == HttpRequest::Method::POST)
    {
        if (request.path == "/register")
            handleRegister(request, response);
        else if (request.path == "/login")
            handleLogin(request, response);
    }

    // const char *response = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\nHello, World!";
    write(m_sockfd, response.serialize().c_str(), response.serialize().length());
    // 关闭连接
    close(m_sockfd);
}

void HttpConn::handleRegister(const HttpRequest &request, HttpResponse &response)
{
    // 从请求中获取用户名和密码
    std::string username = request.getParam("username");
    std::string password = request.getParam("password");

    // 从数据库连接池中获取一个连接
    sqlite3 *conn = SqlConnectionPool::getInstance()->getConnection();
    if (conn == nullptr)
    {
        // 如果获取连接失败，返回500 Internal Server Error
        response = HttpResponse::makeErrorResponse(500);
        return;
    }

    // 构造SQL查询
    std::string sql = "INSERT INTO users (username, password) VALUES ('" + username + "', '" + password + "')";

    // 执行SQL查询
    char *errmsg;
    if (sqlite3_exec(conn, sql.c_str(), nullptr, nullptr, &errmsg) != SQLITE_OK)
    {
        // 如果查询失败，返回500 Internal Server Error，并打印错误信息
        std::cerr << "SQL error: " << errmsg << std::endl;
        sqlite3_free(errmsg);
        response = HttpResponse::makeErrorResponse(500);
    }
    else
    {
        // 如果查询成功，返回200 OK
        response = HttpResponse::makeOkResponse();
    }

    // 将连接返回到数据库连接池
    SqlConnectionPool::getInstance()->returnConnection(conn);
}
