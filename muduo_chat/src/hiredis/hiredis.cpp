#include "hiredis.hpp"
#include "config.hpp"
#include <iostream>
#include <thread>

Redis::Redis() : publish_context_(nullptr), subscribe_context_(nullptr) {}

Redis::~Redis()
{
    if (publish_context_)  redisFree(publish_context_);
    if (subscribe_context_) redisFree(subscribe_context_);
}

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
    // 独立线程阻塞收订阅消息
    std::thread t([this]() { observer_channel_msg(); });
    t.detach();
    std::cout << "connect redis success" << std::endl;
    return true;
}

bool Redis::publish(int channel, const std::string& msg)
{
    std::lock_guard<std::mutex> lock(pub_mtx_);   // 多线程串行化同一个 publish_context_
    redisReply *reply = (redisReply *)redisCommand(publish_context_, "publish %d %s", channel, msg.c_str());
    if (reply == nullptr)
    {
        std::cerr << "publish failed, ch=" << channel << std::endl;
        return false;
    }
    freeReplyObject(reply);
    return true;
}

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
    return true;
}

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
    return true;
}

void Redis::observer_channel_msg()
{
    redisReply *reply = nullptr;
    while (REDIS_OK == redisGetReply(subscribe_context_, (void **)&reply))
    {
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
    std::cerr << "observer_channel_quit (redis disconnected)" << std::endl;
}

void Redis::init_notify_handler(std::function<void(int, std::string)> fn)
{
    notify_handler_ = fn;
}
