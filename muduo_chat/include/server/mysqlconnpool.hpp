#pragma once
#include <mysql/mysql.h>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <string>
#include "config.hpp"

/**
 * @brief MySQL 连接池单例
 * @details 启动时预建 N 条长连接，业务侧 borrow/return，
 *          避免每条消息都短连接握手；get() 时用 mysql_ping 探活，断了自动重连
 */
class MySQLConnPool
{
public:
    static MySQLConnPool& instance();
    MYSQL* get();                 // 取一条连接（池空阻塞等待）
    void release(MYSQL* conn);    // 归还连接
    ~MySQLConnPool();

private:
    MySQLConnPool();
    MySQLConnPool(const MySQLConnPool&) = delete;
    MySQLConnPool& operator=(const MySQLConnPool&) = delete;

    std::queue<MYSQL*> connQueue_;
    std::mutex mtx_;
    std::condition_variable cv_;
    int poolSize_;
};
