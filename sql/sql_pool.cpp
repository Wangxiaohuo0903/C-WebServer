#include "sql_pool.h"
#include <sqlite3.h>
#include <iostream>

SqlConnectionPool *SqlConnectionPool::getInstance()
{
    static SqlConnectionPool instance;
    return &instance;
}

SqlConnectionPool::SqlConnectionPool()
{
    for (int i = 0; i < 8; ++i)
    {
        sqlite3 *conn;
        if (sqlite3_open("test.db", &conn) != SQLITE_OK)
        {
            std::cerr << "Error opening database: " << sqlite3_errmsg(conn) << std::endl;
            return;
        }
        m_connections.push(conn);
    }
}

sqlite3 *SqlConnectionPool::getConnection()
{
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_connections.empty())
    {
        std::cerr << "Out of connections.\n";
        return nullptr;
    }
    sqlite3 *conn = m_connections.front();
    m_connections.pop();
    return conn;
}

void SqlConnectionPool::returnConnection(sqlite3 *conn)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_connections.push(conn);
}
