#include "chatserver.hpp"
#include "chatservice.hpp"
#include "config.hpp"
#include <exception>
#include <iostream>
#include <muduo/base/Timestamp.h>

void chatserver::onConnectionCallback(const TcpConnectionPtr &conn)
{
    if (!conn->connected())
    {
        int id = chatservice::init()->get_id(conn);
        chatservice::init()->clientclose(conn, id);
        std::cout << "client offline, id=" << id << std::endl;
    }
}

void chatserver::MessageCallback(const TcpConnectionPtr &conn, Buffer *buf, Timestamp)
{
    std::string buff = buf->retrieveAllAsString();
    try
    {
        json js = json::parse(buff);
        chatservice::init()->updateActiveTime(conn);

        // 心跳包：只刷新活跃时间，不进业务分发
        if (js.value("msg", "") == "ping") return;

        std::string msgid = js.value("msgid", "");
        if (!msgid.empty())
        {
            fun cb = chatservice::init()->headlrback(msgid);
            cb(conn, js, Timestamp());
        }
    }
    catch (const std::exception &e)
    {
        std::cerr << "json parse error: " << e.what() << std::endl;
    }
}

void chatserver::start()
{
    // 每 15s 做一次空闲连接回收
    server_.getLoop()->runEvery(Config::instance().check_interval_sec,
        []() { chatservice::init()->checkTimeout(); });
    server_.start();
}
