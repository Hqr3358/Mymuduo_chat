#include "hiredis.hpp"
#include "config.hpp"
#include <iostream>
#include <thread>
#include <unistd.h>

Redis::Redis() : publish_context_(nullptr), subscribe_context_(nullptr) {}

Redis::~Redis()
{
    if (publish_context_)  redisFree(publish_context_);
    if (subscribe_context_) redisFree(subscribe_context_);
}

// 连接 Redis：发布和订阅各一条连接，订阅连接单独开线程阻塞收消息
bool Redis::connect()
{
    auto &c = Config::instance();
    publish_context_ = redisConnect(c.redis_host.c_str(), c.redis_port);
    if (publish_context_ == nullptr || publish_context_->err)
    {
        std::cerr << "connect redis(pub) failed: "
                  << (publish_context_ ? publish_context_->errstr : "null") << std::endl;
        return false;
    }
    subscribe_context_ = redisConnect(c.redis_host.c_str(), c.redis_port);
    if (subscribe_context_ == nullptr || subscribe_context_->err)
    {
        std::cerr << "connect redis(sub) failed: "
                  << (subscribe_context_ ? subscribe_context_->errstr : "null") << std::endl;
        return false;
    }
    // 独立线程阻塞收订阅消息，断连会在该线程内自动重连
    std::thread t([this]() { observer_channel_msg(); });
    t.detach();
    std::cout << "connect redis success" << std::endl;
    return true;
}

// 向指定 channel 发布消息（跨机消息路由用），多线程调用加锁
bool Redis::publish(int channel, const std::string& msg)
{
    std::lock_guard<std::mutex> lock(pub_mtx_);   // hiredis 同步连接非线程安全
    redisReply *reply = (redisReply *)redisCommand(publish_context_, "publish %d %s", channel, msg.c_str());
    if (reply == nullptr)
    {
        std::cerr << "publish failed, ch=" << channel << std::endl;
        return false;
    }
    freeReplyObject(reply);
    return true;
}

// 订阅某个 channel（一般就是 userid），同时记录下来，断连重连后自动补订
bool Redis::subscribe(int channel)
{
    if (REDIS_ERR == redisAppendCommand(subscribe_context_, "subscribe %d", channel))
    {
        std::cerr << "subscribe append failed" << std::endl;
        return false;
    }
    int done = 0;
    while (!done)
    {
        if (REDIS_ERR == redisBufferWrite(subscribe_context_, &done))
        {
            std::cerr << "subscribe buffer write failed" << std::endl;
            return false;
        }
    }
    {
        std::lock_guard<std::mutex> lock(sub_mtx_);
        subscribed_channels_.insert(channel);
    }
    return true;
}

// 退订某个 channel，同时从记录集合里删掉
bool Redis::unsubscribe(int channel)
{
    if (REDIS_ERR == redisAppendCommand(subscribe_context_, "unsubscribe %d", channel))
    {
        std::cerr << "unsubscribe append failed" << std::endl;
        return false;
    }
    int done = 0;
    while (!done)
    {
        if (REDIS_ERR == redisBufferWrite(subscribe_context_, &done))
        {
            std::cerr << "unsubscribe buffer write failed" << std::endl;
            return false;
        }
    }
    {
        std::lock_guard<std::mutex> lock(sub_mtx_);
        subscribed_channels_.erase(channel);
    }
    return true;
}

// 重新建立订阅连接，并把之前订阅过的 channel 全部重新订阅一遍
bool Redis::reconnect_sub()
{
    if (subscribe_context_) { redisFree(subscribe_context_); subscribe_context_ = nullptr; }

    auto &c = Config::instance();
    subscribe_context_ = redisConnect(c.redis_host.c_str(), c.redis_port);
    if (subscribe_context_ == nullptr || subscribe_context_->err)
    {
        std::cerr << "redis reconnect(sub) failed: "
                  << (subscribe_context_ ? subscribe_context_->errstr : "null") << std::endl;
        return false;
    }
    // 把之前订阅过的 channel 全部补订
    std::set<int> chans;
    {
        std::lock_guard<std::mutex> lock(sub_mtx_);
        chans = subscribed_channels_;
    }
    for (int ch : chans)
    {
        if (REDIS_ERR == redisAppendCommand(subscribe_context_, "subscribe %d", ch)) continue;
        int done = 0;
        while (!done)
        {
            if (REDIS_ERR == redisBufferWrite(subscribe_context_, &done)) break;
        }
    }
    std::cout << "redis resubscribe " << chans.size() << " channels after reconnect" << std::endl;
    return true;
}

// 独立接收线程：阻塞收订阅消息，底层连接断开后自动 sleep 2s 重连，再继续收
void Redis::observer_channel_msg()
{
    redisReply *reply = nullptr;
    while (true)   // 外层循环：断了就重连
    {
        while (REDIS_OK == redisGetReply(subscribe_context_, (void **)&reply))
        {
            // 订阅推送固定是三元素数组：["message", channel, payload]
            if (reply->type != REDIS_REPLY_ARRAY || reply->elements != 3)
            {
                std::cerr << "unexpected reply: type=" << reply->type
                          << " elements=" << reply->elements << std::endl;
                freeReplyObject(reply);
                continue;
            }
            if (reply->element[1] != nullptr && reply->element[1]->str != nullptr
                && reply->element[2] != nullptr && reply->element[2]->str != nullptr)
            {
                if (notify_handler_)
                    notify_handler_(atoi(reply->element[1]->str), reply->element[2]->str);
            }
            freeReplyObject(reply);
        }
        // 走到这里说明连接断了，清掉残留 reply，等 2 秒重连
        if (reply) freeReplyObject(reply);
        std::cerr << "redis sub disconnected, reconnect after 2s..." << std::endl;
        sleep(2);
        if (!reconnect_sub()) continue;   // 重连失败继续等下一轮
    }
}

void Redis::init_notify_handler(std::function<void(int, std::string)> fn)
{
    notify_handler_ = fn;
}
