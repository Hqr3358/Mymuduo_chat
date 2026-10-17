#pragma once
#include <hiredis/hiredis.h>
#include <functional>
#include <string>
#include <mutex>
#include <set>

/**
 * @brief Redis 发布订阅封装
 * @details publish_context_ 被多个 muduo worker 线程并发使用，加锁串行化；
 *          subscribe_context_ 只在独立接收线程里使用；
 *          订阅线程断连后自动重连，并把之前订阅过的 channel 重新订阅一遍
 */
class Redis
{
public:
    Redis();
    ~Redis();
    bool connect();                                      // 建立发布/订阅两条连接，启动接收线程
    bool publish(int channel, const std::string& msg);   // 向指定 channel 发布消息
    bool subscribe(int channel);                         // 订阅某个 channel（一般是 userid）
    bool unsubscribe(int channel);                       // 退订某个 channel
    void observer_channel_msg();                         // 独立线程里阻塞收订阅消息，断连自动重连
    void init_notify_handler(std::function<void(int, std::string)> fn); // 注册收到消息后的回调
private:
    bool reconnect_sub(); // 重新建立订阅连接并补订之前所有 channel

    redisContext* publish_context_;   // 发布用连接（多线程共享，pub_mtx_ 保护）
    redisContext* subscribe_context_; // 订阅用连接（仅接收线程访问）
    std::function<void(int, std::string)> notify_handler_; // 收到消息后的业务回调
    std::mutex pub_mtx_;              // 保护 publish_context_
    std::mutex sub_mtx_;              // 保护 subscribed_channels_
    std::set<int> subscribed_channels_; // 记录当前订阅过的所有 channel，重连后补订用
};
