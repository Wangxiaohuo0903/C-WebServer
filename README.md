# C-WebServer

C-WebServer是一个使用C++编写的简单Web服务器项目。它采用了多线程和非阻塞I/O模型，以实现高效的并发处理。此外，它还使用了SQLite数据库来处理用户的注册和登录请求。

## 主要特性

- **多线程和非阻塞I/O**: 通过使用多线程和非阻塞I/O，C-WebServer能够同时处理多个请求，从而提高了服务器的性能。

- **SQLite数据库**: C-WebServer使用SQLite数据库来存储用户信息。当用户进行注册或登录操作时，服务器会查询数据库，以验证用户的信息。

- **HTTP请求处理**: C-WebServer能够处理基本的HTTP GET和POST请求。对于GET请求，服务器会返回请求的资源；对于POST请求，服务器会处理用户的注册和登录操作。

- **简单的用户注册和登录**: 用户可以通过发送POST请求来进行注册和登录。注册时，用户需要提供用户名和密码；登录时，用户需要提供用户名和密码，服务器会验证这些信息，并返回相应的响应。

## 使用方法

首先，克隆项目到本地：

```
git clone https://github.com/Wangxiaohuo0903/C-WebServer.git
```

然后，进入项目目录，编译项目：

```
cd C-WebServer
make
```

最后，运行服务器：

```
./server
```

现在，服务器已经在本地运行，你可以通过浏览器或其他HTTP客户端来访问。

```
 webbench -t 5 -c 5 http://localhost:12346/
Webbench - Simple Web Benchmark 1.5
Copyright (c) Radim Kolar 1997-2004, GPL Open Source Software.

Request:
GET / HTTP/1.0
User-Agent: WebBench 1.5
Host: localhost


Runing info: 5 clients, running 5 sec.

Speed=195480 pages/min, 234576 bytes/sec.
Requests: 16290 susceed, 0 failed.

```
## 未来工作

虽然C-WebServer已经实现了基本的功能，但还有许多可以改进和扩展的地方。例如，可以添加更多的HTTP方法支持，如PUT、DELETE等；可以增加对HTTPS的支持；可以增加对更复杂的HTTP请求的处理，如文件上传、Cookie和Session的处理等。

