#pragma once
#include"groupuser.hpp"
#include <vector>
class Group//管理群的属性类
{
public:
    void setgroupid(int groupid);
    void setgroupname(std::string groupname);
    void setgroupdesc(std::string groupdesc);
    //void setgroupusers(std::vector<Groupuser>& users);
    int getgroupid();
    //std::vector<Groupuser>& getuser();//获得保存群组员的容器，这里要用引用,确保该容器一直都是该群的
    std::string getgroupname();
    std::string getgroupdesc();
private:
    int id_;//群id
    std::string name_;//群名称
    std::string desc_;//群的描述
    std::vector<Groupuser> users_;//该群对应的groupuser表
};