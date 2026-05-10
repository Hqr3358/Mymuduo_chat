#include "groupmodel.hpp"
bool Groupmodel::creategroup(int userid, Group &group) // 创建群
{
    char sql[64];
    std::sprintf(sql, "insert into allgroup(groupname,groupdesc) values('%s','%s')", group.getgroupname().c_str(), group.getgroupdesc().c_str());
    Mysql mysql;
    if (mysql.insert(sql))
    {
        group.setgroupid(mysql_insert_id(mysql.getmy_()));
        std::sprintf(sql, "insert into groupuser values(%d,%d,'%s')", group.getgroupid(), userid, "old");
        if (mysql.insert(sql))
        {
            return true;
        }
    }
    return false;
}
bool Groupmodel::insertuser(int groupid, int userid) // 将该userid加入对应群id
{
    char sql[64];
    Mysql mysql;
    std::sprintf(sql, "insert into groupuser values(%d,%d,'%s')", groupid, userid, "normal");
    if (mysql.insert(sql))
    {
        return true;
    }
    return false;
}
std::vector<Groupuser> Groupmodel::query(int groupid, int userid) // 获得该groupid的群组员和群身份
{
    char sql[1024];
    Mysql mysql;
    Groupuser user;
    std::vector<Groupuser> vec;
    std::sprintf(sql, "select userid userrole from groupuser where groupid=%d and userid!=%d",groupid,userid);
    res_ = mysql.query(sql);
    if (res_ != nullptr)
    {
        MYSQL_ROW row;
        while ((row = mysql_fetch_row(res_) )!= nullptr)
        {
            Groupuser user;
            if (row[0] != nullptr)
            {
                user.setid(atoi(row[0]));
            }
            if(row[1]!=nullptr)
            {
                user.setgrouprole(row[1]);
            }
            vec.push_back(user);
        }
    }
    mysql_free_result(res_);
    return vec;
}
std::vector<Group> Groupmodel::querygroup(int userid)//获得该userid的所有群
{
    char sql[1024];
    std::vector<Group> vec;
    Mysql mysql;
    std::sprintf(sql, "select a.groupid,a.groupname,a.groupdesc from allgroup as a,groupuser as b where a.groupid=b.groupid and b.userid=%d",userid);
    res_=mysql.query(sql);
    if (res_ != nullptr)
    {
        MYSQL_ROW row;
        while ((row = mysql_fetch_row(res_) )!= nullptr)
        {
            Group group;
            if (row[0] != nullptr)
            {
                group.setgroupid(atoi(row[0]));
            }
            if(row[1]!=nullptr)
            {
                group.setgroupname(row[1]);
            }
            if(row[2]!=nullptr)
            {
                group.setgroupdesc(row[2]);
            }
            vec.push_back(group);
        }
    }
    mysql_free_result(res_);
    return vec;
}
void Groupmodel::remove(int groupid, int userid) // 将该userid移除该groupid群
{
    char sql[64];
    Mysql mysql;
    std::sprintf(sql, "delete from groupuser where groupid=%d and userid=%d", groupid, userid);
    mysql.update(sql);
}
