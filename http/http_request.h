// http_request.h
#pragma once
#ifndef HTTP_REQUEST_H
#define HTTP_REQUEST_H

#include <string>
#include <unordered_map>
#include <map>
#include "../lock.h"
#include <unistd.h>
#include "../log/log.h"
#include "http_request.h"
#include <sstream>
#include <sqlite3.h>
class HttpRequest
{
public:
    enum Method
    {
        GET,
        POST,
        HEAD,
        PUT,
        DELETE,
        TRACE,
        OPTIONS,
        CONNECT,
        PATCH
    };
    enum ParseState
    {
        REQUEST_LINE,
        HEADERS,
        BODY,
        FINISH
    };

    HttpRequest() : method(GET), parseState(REQUEST_LINE) {}

    bool parse(const std::string &text)
    {
        std::istringstream iss(text);
        std::string line;

        // 解析请求行
        if (!std::getline(iss, line))
        {
            return false;
        }
        std::istringstream iss_line(line);
        std::string method;
        if (!(iss_line >> method))
        {
            return false;
        }
        if (method == "GET")
        {
            this->method = GET;
        }
        else if (method == "POST")
        {
            this->body = text.substr(text.find("\r\n\r\n") + 4);
            this->method = POST;
        }
        else
        {
            return false;
        }
        if (!(iss_line >> this->path))
        {
            return false;
        }
        if (!(iss_line >> this->version))
        {
            return false;
        }

        // 解析请求头
        while (std::getline(iss, line) && line != "\r")
        {
            size_t index = line.find(':');
            if (index == std::string::npos)
            {
                return false;
            }
            std::string key = line.substr(0, index);
            std::string value = line.substr(index + 1);
            this->headers[key] = value;
        }

        // 请求体在GET请求中通常为空，所以这里我们不解析请求体
        this->parseState = FINISH;
        return true;
    }

    std::string getParam(const std::string &name) const
    {
        if (method == GET)
        {
            size_t param_start = path.find("?");
            if (param_start == std::string::npos)
            {
                return "";
            }
            std::string query_string = path.substr(param_start + 1);
            std::istringstream iss(query_string);
            std::string token;
            while (std::getline(iss, token, '&'))
            {
                size_t equal_sign = token.find("=");
                if (equal_sign == std::string::npos)
                {
                    continue;
                }
                std::string key = token.substr(0, equal_sign);
                std::string value = token.substr(equal_sign + 1);
                if (key == name)
                {
                    return value;
                }
            }
        }
        else if (method == POST)
        {
            // 提取POST请求的参数
            std::istringstream iss(body);
            std::string token;
            while (std::getline(iss, token, '&'))
            {
                size_t equal_sign = token.find("=");
                if (equal_sign == std::string::npos)
                {
                    continue;
                }
                std::string key = token.substr(0, equal_sign);
                std::string value = token.substr(equal_sign + 1);
                if (key == name)
                {
                    return value;
                }
            }
        }
        return "";
    }

    Method method;
    std::string path;
    std::string version;
    std::unordered_map<std::string, std::string> headers;

private:
    ParseState parseState;
    std::string body;
};

class HttpResponse
{
public:
    HttpResponse() : status_code(200), headers(), body() {}

    void set_status_code(int code)
    {
        status_code = code;
    }

    void set_header(const std::string &name, const std::string &value)
    {
        headers[name] = value;
    }

    void set_body(const std::string &b)
    {
        body = b;
    }

    std::string to_string() const
    {
        std::string response = "HTTP/1.1 " + std::to_string(status_code) + " OK\r\n";
        for (const auto &header : headers)
        {
            response += header.first + ": " + header.second + "\r\n";
        }
        response += "\r\n" + body;
        return response;
    }

    static HttpResponse makeErrorResponse(int status_code, const std::string &error_message = "")
    {
        HttpResponse response;
        response.set_status_code(status_code);
        response.set_header("Content-Type", "text/plain");
        response.set_body("Error " + std::to_string(status_code) + ": " + error_message);
        return response;
    }

    std::string serialize() const
    {
        std::string response = "HTTP/1.1 " + std::to_string(status_code) + " OK\r\n";
        for (const auto &header : headers)
        {
            response += header.first + ": " + header.second + "\r\n";
        }
        response += "\r\n" + body;
        // std::cout << response.c_str() << std::endl;
        return response;
    }

    static HttpResponse makeOkResponse(const std::string &ok_message = "")
    {
        HttpResponse response;
        response.set_status_code(200);
        response.set_header("Content-Type", "text/plain");
        response.set_body("OK 200: " + ok_message);
        return response;
    }

    void setStatusCode(int status_code)
    {
        this->status_code = status_code;
    }

    // 设置HTTP响应的响应体
    void setBody(const std::string &body)
    {
        this->body = body;
    }

private:
    int status_code;
    std::map<std::string, std::string> headers;
    std::string body;
};

#endif // HTTP_REQUEST_H
