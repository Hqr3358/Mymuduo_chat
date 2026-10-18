#include "offlineusermodel.hpp"
#include "mysql.hpp"

bool Offmodel::insert(int id, const std::string& msg) // 保存离线消息
{
    Mysql mysql;
    std::string sql = "insert into offlineusermodel(userid,msg) values("
                    + std::to_string(id) + ",'" + msg + "')";
    return mysql.insert(sql);
}

std::vector<std::string> Offmodel::query(int id) // 拉取该用户全部离线消息
{
    Mysql mysql;
    std::vector<std::string> vec;
    std::string sql = "select msg from offlineusermodel where userid=" + std::to_string(id);
    MYSQL_RES* res = mysql.query(sql);
    if (res != nullptr)
    {
        MYSQL_ROW row;
        while ((row = mysql_fetch_row(res)) != nullptr)
        {
            if (row[0] != nullptr) vec.push_back(row[0]);
        }
    }
    return vec;
}

void Offmodel::remove(int id) // 清除已读离线消息
{
    Mysql mysql;
    std::string sql = "delete from offlineusermodel where userid=" + std::to_string(id);
    mysql.update(sql);
}
