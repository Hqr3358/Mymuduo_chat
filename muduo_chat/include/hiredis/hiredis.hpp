#pragma once
#include <hiredis/hiredis.h>
#include <functional>
#include <string>
#include <mutex>

// Redis 发布订阅封装：
// - publish_context_ 被多个 muduo worker 线程并发使用，加锁串行化 redisCommand
// - subscribe_context_ 只在独立接收线程里用，不需要锁
class Redis
{
public:
    Redis();
    ~Redis();
    bool connect();
    bool publish(int channel, const std::string& msg);
    bool subscribe(int channel);
    bool unsubscribe(int channel);
    void observer_channel_msg(); // 独立线程里收订阅消息
    void init_notify_handler(std::function<void(int, std::string)> fn);
private:
    redisContext* publish_context_;
    redisContext* subscribe_context_;
    std::function<void(int, std::string)> notify_handler_;
    std::mutex pub_mtx_; // 保护 publish_context_ 的并发访问
};
