#include"friends.hpp"
#include"mysql.hpp"
bool Friend::insert(int id,int friendid)//添加好友
{
    char sql1[128];
    std::sprintf(sql1,"insert into friends values(%d,%d)",id,friendid);
    Mysql mysql;
    bool flag1=mysql.insert(sql1);
    char sql2[128];
    std::sprintf(sql2,"insert into friends values(%d,%d)",friendid,id);
    bool flag2=mysql.insert(sql2);
    return (flag1&&flag2);
}
std::vector<std::string> Friend::query(int id)//显示自己的全部好友id
{
    char sql[128];
    std::sprintf(sql,"select friendid from friends where userid=%d",id);
    Mysql mysql;
    std::vector<std::string> vec;
    res_=mysql.query(sql);
    if (res_ != nullptr)
    {
        MYSQL_ROW row;
        std::string my_friend;
        while ((row = mysql_fetch_row(res_) )!= nullptr)
        {
            if (row[0] != nullptr)
            {
                my_friend+=std::string("id:")+row[0];
                vec.push_back(my_friend);
            }
        }
        
    }
    mysql_free_result(res_);
    return vec;
}
