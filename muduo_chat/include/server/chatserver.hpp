#pragma once
#include <muduo/net/TcpServer.h>
#include <muduo/net/EventLoop.h>
#include <nlohmann/json.hpp>
#include <functional>
#include <thread>
using namespace muduo;
using namespace muduo::net;
using json = nlohmann::json;
using namespace std::placeholders;
// 服务器
class chatserver
{
public:
    chatserver(EventLoop *loop,
               const InetAddress &listenAddr,
               const string &nameArg) : server_(loop, listenAddr, nameArg), loop_(loop)
    {
        server_.setThreadNum(std::thread::hardware_concurrency()); // 设置线程数
        server_.setConnectionCallback(std::bind(&chatserver::onConnectionCallback, this, ::_1));
        server_.setMessageCallback(std::bind(&chatserver::MessageCallback, this, ::_1, ::_2, ::_3));
    }
    void start();
    void onConnectionCallback(const TcpConnectionPtr &conn); // 处理连接和断开时的回调函数
    void MessageCallback(const TcpConnectionPtr &conn,
                         Buffer *buf,
                         Timestamp time); // 处理消息读写时的回调函数
    ~chatserver() {}

private:
    TcpServer server_; // 服务器
    EventLoop *loop_;  // epoll事件循环
};