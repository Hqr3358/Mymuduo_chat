#pragma once
#include<string>
#include<mysql/mysql.h>
#include<vector>
class Friend//管理好友属性类
{
public:
    bool insert(int id,int friendid);//添加好友
    std::vector<std::string> query(int id);//显示自己的全部好友id
    bool remove(int id,int friendid);//删除好友
private:
    MYSQL_RES *res_;
};