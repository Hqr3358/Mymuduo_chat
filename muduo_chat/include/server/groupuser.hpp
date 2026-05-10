#pragma once
#include "user.hpp"
class Groupuser : public User // 管理群组成员类
{
public:
    void setgrouprole(std::string grouprole);
    std::string getgrouprole();
private:
    std::string grouprole_;//群里面的角色（群主，普通，管理员等）
};