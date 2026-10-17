#include "friends.hpp"
#include "mysql.hpp"

bool Friend::insert(int id, int friendid) // 双向加好友
{
    Mysql mysql;
    std::string sql1 = "insert into friends(userid,friendid) values("
                     + std::to_string(id) + "," + std::to_string(friendid) + ")";
    bool flag1 = mysql.insert(sql1);
    std::string sql2 = "insert into friends(userid,friendid) values("
                     + std::to_string(friendid) + "," + std::to_string(id) + ")";
    bool flag2 = mysql.insert(sql2);
    return flag1 && flag2;
}

std::vector<std::string> Friend::query(int id) // 查该用户全部好友 id
{
    Mysql mysql;
    std::vector<std::string> vec;
    std::string sql = "select friendid from friends where userid=" + std::to_string(id);
    MYSQL_RES* res = mysql.query(sql);
    if (res != nullptr)
    {
        MYSQL_ROW row;
        while ((row = mysql_fetch_row(res)) != nullptr)
        {
            if (row[0] != nullptr)
            {
                vec.push_back("id:" + std::string(row[0])); // 每条独立 push，不跨轮次累积
            }
        }
    }
    return vec;
}
