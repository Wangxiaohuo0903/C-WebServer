// http_request.h
#ifndef HTTP_REQUEST_H
#define HTTP_REQUEST_H

#include <string>
#include <unordered_map>
#include "../lock.h"
#include <unistd.h>
#include "../log/log.h"
#include "http_request.h"
#include <sstream>
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
        if (!(iss_line >> this->uri))
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
    std::string uri;
    std::string version;
    std::unordered_map<std::string, std::string> headers;

private:
    ParseState parseState;
};

#endif // HTTP_REQUEST_H
