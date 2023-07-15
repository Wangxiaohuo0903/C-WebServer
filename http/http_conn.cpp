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

void HttpConn::init(int sockfd) : sql_pool(SqlConnectionPool::Instance())
{
    m_sockfd = sockfd;
    // memset(m_read_buf, 0, READ_BUFFER_SIZE);
}

void HttpConn::process()
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
    // if (strncmp(m_read_buf, "GET", 3) != 0)
    if (!request.parse(m_read_buf))
    {
        // 不是GET请求，返回400 Bad Request
        const char *response = "HTTP/1.1 400 Bad Request\r\n\r\n";
        write(m_sockfd, response, strlen(response));
        close(m_sockfd);
        return;
    }

    // 生成响应
    char *response;
    if (request.method == HttpRequest::Method::POST && request.path == "/register")
    {
        std::string username = request.get_param("username");
        std::string password = request.get_param("password");

        sqlite3 *db = sql_pool.get_connection();
        std::string sql = "INSERT INTO users (username, password) VALUES (?, ?)";
        sqlite3_stmt *stmt;
        sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, NULL);
        sqlite3_bind_text(stmt, 1, username.c_str(), -1, SQLITE_STATIC);
        sqlite3_bind_text(stmt, 2, password.c_str(), -1, SQLITE_STATIC);
        sqlite3_step(stmt);
        sqlite3_finalize(stmt);
        sql_pool.return_connection(db);

        response.status_code = 200;
        response.body = "Register success";
    }

    // const char *response = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\nHello, World!";
    write(m_sockfd, response, strlen(response));
    // 关闭连接
    close(m_sockfd);
}
