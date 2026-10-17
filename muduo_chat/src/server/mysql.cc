#include "mysql.hpp"
#include "mysqlconnpool.hpp"
#include <iostream>

Mysql::Mysql()
{
    my_ = MySQLConnPool::instance().get();   // 从连接池借一个长连接
    res_ = nullptr;
}

Mysql::~Mysql()
{
    // 归还前必须释放残留结果集，否则下一个使用者拿到脏连接
    if (res_ != nullptr) { mysql_free_result(res_); res_ = nullptr; }
    MySQLConnPool::instance().release(my_);
}

MYSQL* Mysql::getmy_() { return my_; }

bool Mysql::insert(std::string sql)
{
    if (mysql_query(my_, sql.c_str()) != 0)
    {
        std::cerr << "[INSERT FAIL] " << sql << " err: " << mysql_error(my_) << std::endl;
        return false;
    }
    return true;
}

MYSQL_RES* Mysql::query(std::string sql)
{
    if (mysql_query(my_, sql.c_str()) != 0)
    {
        std::cerr << "[QUERY FAIL] " << sql << " err: " << mysql_error(my_) << std::endl;
        return nullptr;
    }
    res_ = mysql_store_result(my_);
    return res_;
}

void Mysql::update(std::string sql)
{
    if (mysql_query(my_, sql.c_str()) != 0)
    {
        std::cerr << "[UPDATE FAIL] " << sql << " err: " << mysql_error(my_) << std::endl;
    }
}
