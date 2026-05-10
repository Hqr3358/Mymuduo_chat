#pragma once
#include"group.hpp"
#include"mysql.hpp"
#include<mutex>
class Groupmodel//关于群具体类
{
public:
    bool creategroup(int userid,Group& group);//创建群
    bool insertuser(int groupid,int userid);//将该userid加入该groupid群
    void remove(int groupid,int userid);//将该userid移除该groupid群
    std::vector<Groupuser> query(int groupid,int userid);// 获得该groupid的群组员和群身份
    std::vector<Group>querygroup(int userid);//获得该userid的所有群

private:
    std::mutex mtx_;//在多线程中保证线程安全
    MYSQL_RES *res_;
};