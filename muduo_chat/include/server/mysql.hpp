#pragma once
#include<mysql/mysql.h>
#include<string>
class Mysql
{
public:
    Mysql();
    MYSQL* getmy_();
    MYSQL_RES* query(std::string sql);//将该表对应列显示出来
    bool insert(std::string sql);//插入新数据到对应表中
    void update(std::string sql);//更新数据到对应表中
private:
    MYSQL* my_;
    MYSQL_RES* res_;
};