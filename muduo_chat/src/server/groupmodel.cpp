#include "groupmodel.hpp"
#include "mysql.hpp"

bool Groupmodel::creategroup(int userid, Group &group) // 创建群：插 allgroup + 把创建者设为群主
{
    Mysql mysql;
    std::string sql = "insert into allgroup(groupname,groupdesc) values('"
                    + group.getgroupname() + "','" + group.getgroupdesc() + "')";
    if (!mysql.insert(sql)) return false;

    group.setgroupid(mysql_insert_id(mysql.getmy_()));
    std::string sql2 = "insert into groupuser(groupid,userid,grouprole) values("
                     + std::to_string(group.getgroupid()) + ","
                     + std::to_string(userid) + ",'old')";
    if (!mysql.insert(sql2)) return false;   // 两步都成功才算建群成功（走同一连接）
    return true;
}

bool Groupmodel::insertuser(int groupid, int userid) // 用户加入群
{
    Mysql mysql;
    std::string sql = "insert into groupuser(groupid,userid,grouprole) values("
                    + std::to_string(groupid) + "," + std::to_string(userid) + ",'normal')";
    return mysql.insert(sql);
}

std::vector<Groupuser> Groupmodel::query(int groupid, int userid) // 查群内除自己外所有成员
{
    Mysql mysql;
    std::vector<Groupuser> vec;
    // 修复原 bug：select userid userrole 少逗号，会导致整条 SQL 失败
    std::string sql = "select userid, grouprole from groupuser where groupid="
                    + std::to_string(groupid) + " and userid!=" + std::to_string(userid);
    MYSQL_RES* res = mysql.query(sql);
    if (res != nullptr)
    {
        MYSQL_ROW row;
        while ((row = mysql_fetch_row(res)) != nullptr)
        {
            Groupuser u;
            if (row[0] != nullptr) u.setid(atoi(row[0]));
            if (row[1] != nullptr) u.setgrouprole(row[1]);
            vec.push_back(u);
        }
    }
    return vec;
}

std::vector<Group> Groupmodel::querygroup(int userid) // 查该用户加入的全部群
{
    Mysql mysql;
    std::vector<Group> vec;
    std::string sql = "select a.groupid,a.groupname,a.groupdesc from allgroup as a, groupuser as b "
                     "where a.groupid=b.groupid and b.userid=" + std::to_string(userid);
    MYSQL_RES* res = mysql.query(sql);
    if (res != nullptr)
    {
        MYSQL_ROW row;
        while ((row = mysql_fetch_row(res)) != nullptr)
        {
            Group g;
            if (row[0] != nullptr) g.setgroupid(atoi(row[0]));
            if (row[1] != nullptr) g.setgroupname(row[1]);
            if (row[2] != nullptr) g.setgroupdesc(row[2]);
            vec.push_back(g);
        }
    }
    return vec;
}

void Groupmodel::remove(int groupid, int userid) // 用户退群
{
    Mysql mysql;
    std::string sql = "delete from groupuser where groupid=" + std::to_string(groupid)
                    + " and userid=" + std::to_string(userid);
    mysql.update(sql);
}
