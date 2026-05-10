#include"group.hpp"

void Group::setgroupid(int groupid)
{
    id_=groupid;
}
void Group::setgroupname(std::string groupname)
{
    name_=groupname;
}
void Group::setgroupdesc(std::string groupdesc)
{
    desc_=groupdesc;
}
// void Group::setgroupusers(std::vector<Groupuser>& users)
// {
//     users_=users;
// }
int Group::getgroupid()
{
    return id_;
}
// std::vector<Groupuser>& Group::getuser()
// {
//     return users_;
// }
std::string Group::getgroupname()
{
   return name_;
}
std::string Group::getgroupdesc()
{
    return desc_;
}






