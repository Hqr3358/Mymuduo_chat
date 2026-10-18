#include "usermodel.hpp"
#include "mysql.hpp"
#include <string>

UserModel::UserModel() {}

bool UserModel::insert(User &us) // 插入新用户
{
    Mysql mysql;
    // 用 std::string 拼接避免 sprintf 栈溢出；注意：生产应改预处理语句防注入
    std::string sql = "insert into user(name,passwd) values('"
                    + us.getname() + "','" + us.getpassword() + "')";
    if (mysql.insert(sql))
    {
        us.setid(mysql_insert_id(mysql.getmy_()));
        return true;
    }
    return false;
}

User UserModel::query(int id) // 查询用户信息
{
    Mysql mysql;
    User user;
    std::string sql = "select id,name,state,passwd from user where id=" + std::to_string(id);
    MYSQL_RES* res = mysql.query(sql);
    if (res != nullptr)
    {
        MYSQL_ROW row = mysql_fetch_row(res);
        if (row != nullptr)
        {
            if (row[0] != nullptr) user.setid(atoi(row[0]));
            if (row[1] != nullptr) user.setname(row[1]);
            if (row[2] != nullptr) user.setstate(row[2]);
            if (row[3] != nullptr) user.setpassword(row[3]);
        }
        // res 由 Mysql 析构统一 free，这里不再手动 free
    }
    return user;
}

void UserModel::update(User &user) // 修改用户状态等信息
{
    Mysql mysql;
    std::string sql = "update user set state='" + user.getstate() + "' where id=" + std::to_string(user.getid());
    mysql.update(sql);
}

void UserModel::offline(int id) // 用户下线
{
    Mysql mysql;
    std::string sql = "update user set state='offonline' where id=" + std::to_string(id);
    mysql.update(sql);
}

void UserModel::resetstate() // 服务端退出时把所有 online 置为 offline
{
    Mysql mysql;
    std::string sql = "update user set state='offonline' where state='online'";
    mysql.update(sql);
}
