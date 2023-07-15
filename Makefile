# 编译器
CC = g++

# 编译选项
CFLAGS = -Wall -std=c++11 -pthread

# 目标文件
TARGET = server

# 源文件
SOURCES = main.cpp http/http_conn.cpp server.cpp log/log.cpp sql/sql_pool.cpp

# 对象文件
OBJECTS = $(SOURCES:.cpp=.o)

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CC) $(CFLAGS) -o $@ $^ -lsqlite3

%.o: %.cpp
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJECTS) $(TARGET)
