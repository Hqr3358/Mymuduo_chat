#pragma once
#include<string>
class User//用户信息
{
public:
    User();
    //保存用户的信息
    void setid(int id);
    void setname(std::string name);
    void setstate(std::string state);
    void setpassword(std::string ps);
    void settime(time_t t);
    //获取用户的信息
    int getid();
    std::string  getname();
    std::string  getstate();
    std::string  getpassword();

private:
    int id_;//用户id,用来usermodel查找该用户
    std::string name_;//用户名
    std::string state_;//用户在线状态
    std::string ps_;//用户密码
    time_t ti_;//上次活跃时间（包括ping验证）
};