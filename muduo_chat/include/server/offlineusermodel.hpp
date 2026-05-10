#pragma once
#include"mysql.hpp"
#include<vector>
class Offmodel//对用户保存的消息进行管理
{   
public:
    bool insert(int id,std::string& msg);//将离线消息保存在表中的对应id用户上
    std::vector<std::string> query(int id);//将保存的消息显示给该id用户
    void remove(int id);//清除已看过的离线消息
private:
    MYSQL_RES *res_;
   // std::string msg_;
};