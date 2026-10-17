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
    MYSQL* getmy_();                              // 获取底层 MYSQL* 句柄
    MYSQL_RES* query(std::string sql);            // 查询，返回结果集（由析构统一释放）
    bool insert(std::string sql);                 // insert，成功返回 true
    void update(std::string sql);                 // update/delete，无返回值
private:
    MYSQL* my_;
    MYSQL_RES* res_;
};
