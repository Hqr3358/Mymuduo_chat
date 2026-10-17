#include "mysqlconnpool.hpp"
#include <iostream>

MySQLConnPool& MySQLConnPool::instance()
{
    static MySQLConnPool pool;
    return pool;
}

MySQLConnPool::MySQLConnPool()
{
    auto &c = Config::instance();
    poolSize_ = c.mysql_poolsize;
    for (int i = 0; i < poolSize_; ++i)
    {
        MYSQL* conn = mysql_init(nullptr);
        if (conn == nullptr) { std::cerr << "mysql_init failed"; continue; }
        if (mysql_real_connect(conn, c.mysql_host.c_str(), c.mysql_user.c_str(),
                               c.mysql_passwd.c_str(), c.mysql_db.c_str(),
                               c.mysql_port, nullptr, 0) == nullptr)
        {
            std::cerr << "mysql_real_connect failed: " << mysql_error(conn);
            mysql_close(conn);
            continue;
        }
        connQueue_.push(conn);
    }
    if (connQueue_.empty()) std::cerr << "MySQLConnPool: no connection available!";
    else std::cout << "MySQLConnPool init, pool size=" << connQueue_.size();
}

MYSQL* MySQLConnPool::get()
{
    std::unique_lock<std::mutex> lock(mtx_);
    cv_.wait(lock, [this]{ return !connQueue_.empty(); });
    MYSQL* conn = connQueue_.front();
    connQueue_.pop();
    // 检查连接是否被 server 端断开（8小时超时），断了就重建
    if (mysql_ping(conn) != 0)
    {
        std::cerr << "mysql_ping failed, reconnect: " << mysql_error(conn);
        if (mysql_real_connect(conn, Config::instance().mysql_host.c_str(),
                               Config::instance().mysql_user.c_str(),
                               Config::instance().mysql_passwd.c_str(),
                               Config::instance().mysql_db.c_str(),
                               Config::instance().mysql_port, nullptr, 0) == nullptr)
        {
            std::cerr << "reconnect failed: " << mysql_error(conn);
        }
    }
    return conn;
}

void MySQLConnPool::release(MYSQL* conn)
{
    if (!conn) return;
    std::lock_guard<std::mutex> lock(mtx_);
    connQueue_.push(conn);
    cv_.notify_one();
}

MySQLConnPool::~MySQLConnPool()
{
    std::lock_guard<std::mutex> lock(mtx_);
    while (!connQueue_.empty())
    {
        MYSQL* conn = connQueue_.front();
        connQueue_.pop();
        mysql_close(conn);
    }
}
