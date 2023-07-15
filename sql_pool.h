#include <queue>
#include <sqlite3.h>

class SqlConnectionPool
{
public:
    SqlConnectionPool(int size)
    {
        for (int i = 0; i < size; i++)
        {
            sqlite3 *db;
            sqlite3_open("users.db", &db);
            m_pool.push(db);
        }
    }

    ~SqlConnectionPool()
    {
        while (!m_pool.empty())
        {
            sqlite3_close(m_pool.front());
            m_pool.pop();
        }
    }

    sqlite3 *get_connection()
    {
        sqlite3 *db = m_pool.front();
        m_pool.pop();
        return db;
    }

    void return_connection(sqlite3 *db)
    {
        m_pool.push(db);
    }

private:
    std::queue<sqlite3 *> m_pool;
};
