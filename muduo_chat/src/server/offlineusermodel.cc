#include "offlineusermodel.hpp"

bool Offmodel::insert(int id,std::string &msg) // 将离线消息保存在表中的对应id用户上
{
    Mysql mysql;
    char sql[128];
    std::sprintf(sql, "insert into offlineusermodel values(%d,'%s')", id, msg.c_str());
    return mysql.insert(sql);
}
std::vector<std::string> Offmodel::query(int id) // 将保存的消息显示给该id用户
{
    Mysql mysql;
    std::vector<std::string> vec;
    char sql[1024];
    std::sprintf(sql, "select msg from offlineusermodel where userid=%d", id);
    res_ = mysql.query(sql);
    if (res_ != nullptr)
    {
        MYSQL_ROW row;
        while ((row = mysql_fetch_row(res_) )!= nullptr)
        {
            if (row[0] != nullptr)
            {
                vec.push_back(row[0]);
            }
        }
    }
    mysql_free_result(res_);
    return vec;
}
void Offmodel::remove(int id) // 清除已看过的离线消息
{
    Mysql mysql;
    char sql[128];
    std::sprintf(sql, "delete from offlineusermodel where userid=%d", id);
    mysql.update(sql);
}
