#pragma once
#include <thread>
#include <chrono>
#include <sys/socket.h>
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
    memset(m_read_buf, 0, READ_BUFFER_SIZE);
}

void HttpConn::readData()
{
    if (m_sockfd == -1)
    {
        return;
    }

    int n = read(m_sockfd, m_read_buf, READ_BUFFER_SIZE - 1);
    if (n < 0)
    {
        if (errno == EAGAIN || errno == EWOULDBLOCK)
        {
            // Resource temporarily unavailable, try again later
            return;
        }
        else
        {
            perror("Error in read()");
            close(m_sockfd);
            m_sockfd = -1;
            return;
        }
    }
    else if (n == 0)
    {
        // Connection closed by client
        close(m_sockfd);
        m_sockfd = -1;
        return;
    }
}

void HttpConn::process()
{
    if (m_sockfd == -1)
    {
        return;
    }

    HttpResponse response;

    // 在 Proactor 模式中，数据的读取应该在任务被添加到队列之前完成
    // 所以在 process 方法中不应该再次读取数据
    if (m_mode == REACTOR)
    {
        int n = read(m_sockfd, m_read_buf, READ_BUFFER_SIZE - 1);
        if (n < 0)
        {
            if (errno == EAGAIN || errno == EWOULDBLOCK)
            {
                // Resource temporarily unavailable, try again later
                return;
            }
            else
            {
                perror("Error in read()");
                close(m_sockfd);
                m_sockfd = -1;
                return;
            }
        }
        else if (n == 0)
        {
            // Connection closed by client
            close(m_sockfd);
            m_sockfd = -1;
            return;
        }
    }

    if (!request.parse(m_read_buf))
    {
        response = HttpResponse::makeErrorResponse(400);
    }

    // 生成响应
    if (request.method == HttpRequest::Method::POST)
    {
        if (request.path == "/register")
        {
            // LOG_INFO("start register");
            handleRegister(request, response);
        }
        else if (request.path == "/login")
        {
            // LOG_INFO("start login");
            handleLogin(request, response);
        }
    }
    else if (request.method == HttpRequest::Method::GET)
    {
        // LOG_INFO("return GET");
        response = HttpResponse::makeOkResponse();
    }

    response.set_header("Connection", "close");
    ssize_t written = write(m_sockfd, response.serialize().c_str(), response.serialize().length());
    if (written < 0)
    {
        perror("Error in write()");
        close(m_sockfd);
        m_sockfd = -1;
        return;
    }

    // 在 Proactor 模式中，数据的读取应该在任务被添加到队列之前完成
    // 所以在 process 方法中不应该再次读取数据
    if (m_mode == REACTOR)
    {
        char buffer[1024];
        if (m_sockfd != -1)
        {
            while (read(m_sockfd, buffer, sizeof(buffer)) > 0)
            {
                // just read until the client closes the connectiçon
            }
        }
    }

    shutdown(m_sockfd, SHUT_WR);
}

// 回调函数，如果被调用，说明查询结果至少有一行，也就是说用户名已经存在
static int callback(void *data, int argc, char **argv, char **azColName)
{
    int *usernameExists = (int *)data;
    *usernameExists = 1;
    return 0;
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
        SqlConnectionPool::getInstance()->returnConnection(conn);
        return;
    }
    // 检查用户名是否已存在
    std::string checkUsernameSQL = "SELECT username FROM users WHERE username = '" + username + "'";
    int usernameExists = 0;
    char *errmsg;
    int result = sqlite3_exec(conn, checkUsernameSQL.c_str(), callback, &usernameExists, &errmsg);
    if (result != SQLITE_OK)
    {
        // 查询失败，打印错误信息并返回500 Internal Server Error
        std::cerr << "SQL error: " << errmsg << std::endl;
        sqlite3_free(errmsg);
        response = HttpResponse::makeErrorResponse(500);
        SqlConnectionPool::getInstance()->returnConnection(conn);
        return;
    }
    if (usernameExists)
    {
        // 用户名已存在，返回自定义的错误响应
        response = HttpResponse::makeOkResponse("Username already exists");
        SqlConnectionPool::getInstance()->returnConnection(conn);
        return;
    }

    // 构造SQL查询
    std::string sql = "INSERT INTO users (username, password) VALUES ('" + username + "', '" + password + "')";

    // 执行SQL查询
    if (sqlite3_exec(conn, sql.c_str(), nullptr, nullptr, &errmsg) != SQLITE_OK)
    {
        // 如果查询失败，返回500 Internal Server Error，并打印错误信息
        std::cerr << "SQLL error: " << errmsg << std::endl;
        sqlite3_free(errmsg);
        response = HttpResponse::makeErrorResponse(500);
    }
    else
    {
        std::cout << "success register" << std::endl;
        // 如果查询成功，返回200 OK
        LOG_INFO("success register");
        response = HttpResponse::makeOkResponse("success register");
    }

    // 将连接返回到数据库连接池
    SqlConnectionPool::getInstance()->returnConnection(conn);
}

// 回调函数，如果被调用，说明查询结果至少有一行，也就是说用户名存在
static int callback1(void *data, int argc, char **argv, char **azColName)
{
    std::string *passwordFromDB = (std::string *)data;
    if (argv[0])
    {
        *passwordFromDB = argv[0];
    }
    // std::cout << "password " << *passwordFromDB << std::endl;
    return 0;
}

void HttpConn::handleLogin(const HttpRequest &request, HttpResponse &response)
{
    // 从HttpRequest中获取用户名和密码
    std::string username = request.getParam("username");
    std::string password = request.getParam("password");

    // 在数据库中查询用户名和密码
    std::string sql = "SELECT password FROM users WHERE username='" + username + "'";
    std::string passwordFromDB;
    char *errmsg;
    // 从数据库连接池中获取一个连接
    sqlite3 *conn = SqlConnectionPool::getInstance()->getConnection();
    if (conn == nullptr)
    {
        // 如果获取连接失败，返回500 Internal Server Error
        response = HttpResponse::makeErrorResponse(500);
        SqlConnectionPool::getInstance()->returnConnection(conn);
        return;
    }
    int rc = sqlite3_exec(conn, sql.c_str(), callback1, &passwordFromDB, &errmsg);
    if (rc != SQLITE_OK)
    {
        // 查询失败，返回500错误
        response = HttpResponse::makeErrorResponse(500);
        std::cout << "500 " << std::endl;
        SqlConnectionPool::getInstance()->returnConnection(conn);
        return;
    }

    if (passwordFromDB.empty())
    {
        // 用户名不存在，返回403错误
        LOG_INFO("500 Username does not exist")
        response = HttpResponse::makeErrorResponse(403, "Username does not exist");
        SqlConnectionPool::getInstance()->returnConnection(conn);
        return;
    }

    // 比较查询到的密码和用户提交的密码
    else if (password != passwordFromDB)
    {
        // 密码不正确，返回403错误
        LOG_INFO("Password is incorrect");
        // response.setStatusCode(200);
        // response.setBody("Password is incorrect");
        response = HttpResponse::makeErrorResponse(403, "Password is incorrect");
        SqlConnectionPool::getInstance()->returnConnection(conn);
        return;
    }

    // 登录成功，返回200状态码和欢迎信息
    response = HttpResponse::makeOkResponse("Welcome, " + username + "!");
    LOG_INFO("Success Login");
    // 将连接返回到数据库连接池
    SqlConnectionPool::getInstance()->returnConnection(conn);
}
