#include "usermodel.hpp"
#include <string>
UserModel::UserModel()
{
}
bool UserModel::insert(User &us) // 插入新用户
{
    Mysql mysql; // 获取该用户在表中的对应id
    char sql[128];
    std::sprintf(sql, "insert into user(name,passwd) values('%s','%s')", us.getname().c_str(), us.getpassword().c_str());
    if (mysql.insert(sql)) // 将用户插入进mysql表中,并完善user_id
    {
        us.setid(mysql_insert_id(mysql.getmy_()));
        return true;
    }
    return false;
}
User UserModel::query(int id) // 进行查询用户信息之类的操作
{
    Mysql mysql;
    User user;
    char sql[128];
    std::sprintf(sql, "select * from user where id=%d", id);
    res_ = mysql.query(sql);
    if (res_ != nullptr)
    {
        MYSQL_ROW row = mysql_fetch_row(res_);
        if (row != nullptr)
        {
            if (row[0] != nullptr)
            {
                user.setid(atoi(row[0]));
            }
            if (row[1] != nullptr)
            {
                user.setname(row[1]);
            }
            if (row[2] != nullptr)
            {
                user.setstate(row[2]);
            }
            if (row[3] != nullptr)
            {
                user.setpassword(row[3]);
            }
        }
    }
    mysql_free_result(res_);
    return user;
}
void UserModel::update(User &user) // 进行修改用户信息之类的操作
{
    Mysql mysql;
    char sql[128];
    std::sprintf(sql, "update user set state='%s' where id=%d", user.getstate().c_str(), user.getid());
    mysql.update(sql);
}
void UserModel::offline(int id) // 用户下线
{
    Mysql mysql;
    char sql[128];
    std::sprintf(sql, "update user set state='offonline' where id=%d", id);
    mysql.update(sql);
}
void UserModel::resetstate() // 服务端异常退出,将在线全变为离线
{
    Mysql mysql;
    char sql[128];
    std::sprintf(sql, "update user set state='offonline' where state='online'");
    mysql.update(sql);
}
