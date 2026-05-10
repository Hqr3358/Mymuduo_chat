#pragma once
#include "user.hpp"
#include "mysql.hpp"
#include <mutex>
class UserModel // 对整体用户进行管理
{
public:
    UserModel();
    bool insert(User &us);   // 插入新用户
    User query(int id);      // 进行显示用户信息之类的操作
    void update(User &user); // 进行修改用户信息之类的操作
    void offline(int id);    // 用户下线
    void resetstate(); // 服务端异常退出,将在线全变为离线
private:
    std::mutex mtx_; // 在多线程中保证线程安全
    MYSQL_RES *res_;
};