// http_request.h
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

    Method method;
    std::string path;
    std::string version;
    std::unordered_map<std::string, std::string> headers;

private:
    ParseState parseState;
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

private:
    int status_code;
    std::map<std::string, std::string> headers;
    std::string body;
};

#endif // HTTP_REQUEST_H
