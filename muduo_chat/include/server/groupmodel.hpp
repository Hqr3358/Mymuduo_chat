#pragma once
#include "group.hpp"
#include <mutex>

struct MYSQL_RES;   // 前向声明，mysql.hpp 只在 .cc 里 include

class Groupmodel // 关于群具体类
{
public:
    bool creategroup(int userid, Group& group);     // 创建群
    bool insertuser(int groupid, int userid);        // 将该 userid 加入该 groupid 群
    void remove(int groupid, int userid);             // 将该 userid 移出该 groupid 群
    std::vector<Groupuser> query(int groupid, int userid);  // 获得该 groupid 的群组员和群身份
    std::vector<Group> querygroup(int userid);             // 获得该 userid 的所有群

private:
    std::mutex mtx_;   // 在多线程中保证线程安全
    MYSQL_RES *res_;
};
