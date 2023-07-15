#include <queue>
#include <sqlite3.h>
#include <pthread.h>
class SqlConnectionPool
{
public:
    // 获取数据库连接池的单例实例
    static SqlConnectionPool *getInstance();

    // 从连接池中获取一个数据库连接
    sqlite3 *getConnection();

    // 将数据库连接归还给连接池
    void returnConnection(sqlite3 *connection);

private:
    // 私有的构造函数，用于实现单例模式
    SqlConnectionPool();

    // 数据库连接池
    std::queue<sqlite3 *> m_connections;

    // 用于保护数据库连接池的互斥锁
    std::mutex m_mutex;
};
