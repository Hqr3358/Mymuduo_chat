#pragma once
#include <mysql/mysql.h>
#include <string>

/**
 * @brief MySQL 连接句柄（RAII）
 * @details 构造时从连接池借一条连接，析构时 free_result 后归还，业务侧用法不变
 */
class Mysql
{
public:
    Mysql();                  // 构造：从连接池取连接
    ~Mysql();                 // 析构：归还连接
    MYSQL* getmy_();
    MYSQL_RES* query(std::string sql);
    bool insert(std::string sql);
    void update(std::string sql);
private:
    MYSQL* my_;
    MYSQL_RES* res_;
};
