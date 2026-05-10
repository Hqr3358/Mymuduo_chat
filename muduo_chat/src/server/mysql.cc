#include"mysql.hpp"
#include<muduo/base/Logging.h>
#include"iostream"
const char* host="localhost";
const char* user="qiu";
const char* passwd="1234";
const char* db="user1";
unsigned int port=3306;
Mysql::Mysql()
{
    my_=mysql_init(nullptr);//初始化数据库
    if(my_==nullptr)
    {
        LOG_ERROR<<"mysql初始化失败";
    }
    if(mysql_real_connect(my_,host,user,passwd,db,port,nullptr,0)==nullptr)//连接数据库服务端
    {
        LOG_ERROR<<"mysql连接服务端失败";
    }
    
   // res_=mysql_store_result(my_);//获取my里面的结果集
}
MYSQL* Mysql::getmy_()
{
    return my_;
}
bool Mysql::insert(std::string sql)//在注册表中插入该用户
{
    int n=mysql_query(my_,sql.c_str());
    if(n!=0)
    {
        std::cerr<<sql<<"mysql insert send faild"<<std::endl;
        return false;
    }
    return true;
}

MYSQL_RES* Mysql::query(std::string sql)//查询用户数据信息
{
    int n=mysql_query(my_,sql.c_str());
    if(n!=0)
    {
        std::cerr<<sql<<"mysql query send faild"<<std::endl;
        return nullptr;
    }
    res_=mysql_store_result(my_);
    return res_;
}
void Mysql::update(std::string sql)
{
    int n=mysql_query(my_,sql.c_str());
    if(n!=0)
    {
        std::cerr<<sql<<"mysql update send faild"<<std::endl;
    }
}

