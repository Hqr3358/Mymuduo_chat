#include "user.hpp"
#include <time.h>
User::User() : id_(0), name_(" "), ps_(" "), state_(" "),ti_(time(nullptr))
{
}
void User::setid(int id)
{
    id_ = id;
}
void User::setname(std::string name)
{
    name_ = name;
}
void User::setstate(std::string state)
{
    state_ = state;
}
void User::setpassword(std::string ps)
{
    ps_ = ps;
}
void User::settime(time_t t)
{
    ti_=t;
}
int User::getid()
{
    return id_;
}
std::string User::getname()
{
    return name_;
}
std::string User::getstate()
{
    return state_;
}
std::string User::getpassword()
{
    return ps_;
}
