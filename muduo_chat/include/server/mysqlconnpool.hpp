#pragma once
#include <mysql/mysql.h>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <string>
#include "config.hpp"

// MySQL 连接池：预先建立 N 个长连接，业务侧借/还，避免每条消息短连接握手
class MySQLConnPool
{
public:
    static MySQLConnPool& instance();

    // 取一个连接（池空则阻塞等待）
    MYSQL* get();
    // 归还连接
    void release(MYSQL* conn);
    ~MySQLConnPool();

private:
    MySQLConnPool();
    MySQLConnPool(const MySQLConnPool&) = delete;
    MySQLConnPool& operator=(const MySQLConnPool&) = delete;

    void produce();   // 启动后台线程补充连接（这里简化：启动时一次性建好）
    void close();

    std::queue<MYSQL*> connQueue_;
    std::mutex mtx_;
    std::condition_variable cv_;
    int poolSize_;
};
