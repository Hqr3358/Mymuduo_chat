#include "chatserver.hpp"
#include "chatservice.hpp"
#include <exception>
#include<iostream>
#include <muduo/base/Timestamp.h>
void chatserver::onConnectionCallback(const TcpConnectionPtr &conn) // 处理连接和断开时的回调函数
{
    //客户端断开时，让用户下线
    if(!conn->connected())
    { 
        int id=chatservice::init()->get_id(conn);
        chatservice::init()->clientclose(conn,id);  
        std::cout<<"客服端: "<<id<<" 下线"<<std::endl;
    }
}
void chatserver::MessageCallback(const TcpConnectionPtr& conn, Buffer *buf, Timestamp time) // 处理消息读写时的回调函数
{
    std::string buff=buf->retrieveAllAsString();
    try 
    {
		json js=json::parse(buff);//将获得的数据json序列化
    	chatservice::init()->updateActiveTime(conn);
    	std::string msg=js.value("msg","");
    	int msgid=js.value("msgid",-1);
    	if(msg=="ping") return;
    	//虽然是单例不能并发访问该对象的函数，但可以利用回调让这个可以并发的muduo处理读写函数访问，从而实现并发
    	if(msgid!=-1)
    	{
        	fun msgback=chatservice::init()->headlrback(js["msgid"].get<int>());//回调获取对应的服务执行函数
    		msgback(conn,js,time);//执行对应的服务函数
    	}
    } catch (const std::exception& e) 
    {
    	std::cerr<<"json error->"<<e.what()<<std::endl;
        return;
    }
}
void chatserver::start()//启动服务器
{
    server_.getLoop()->runEvery(15.0, []()
	{
		chatservice::init()->checkTimeout();
	});	
    server_.start();
}