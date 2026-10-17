#pragma once
#include <mysql/mysql.h>
#include <string>

// Mysql：从连接池借一个连接，析构自动归还（不 close）
// 上层 Model 用法不变：Mysql mysql; mysql.query(...);
class Mysql
{
public:
    Mysql();                  // 构造时从池里取连接
    ~Mysql();                 // 析构时 free_result 并归还连接
    MYSQL* getmy_();
    MYSQL_RES* query(std::string sql);
    bool insert(std::string sql);
    void update(std::string sql);
private:
    MYSQL* my_;
    MYSQL_RES* res_;
};
