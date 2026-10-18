#pragma once
#include <vector>
#include <string>

struct MYSQL_RES;   // 前向声明，mysql.hpp 只在 .cc 里 include

class Offmodel // 离线消息表管理
{
public:
    bool insert(int id, const std::string& msg); // 存离线消息
    std::vector<std::string> query(int id);       // 拉全部离线消息
    void remove(int id);                          // 清空已读
private:
    MYSQL_RES *res_;
};
