#include "hiredis.hpp"
#include <iostream>
#include<thread>
Redis::Redis() : publish_context_(nullptr), subscribe_context_(nullptr)
{
}
Redis::~Redis()
{
    redisFree(publish_context_);
    redisFree(subscribe_context_);
}
bool Redis::connect() // 连接redis服务端
{
    publish_context_ = redisConnect("127.0.0.1", 6379); // 负责发布消息的上下文连接
    if (publish_context_ == nullptr)
    {
        std::cerr << "connent redis failed" << std::endl;
        return false;
    }
    subscribe_context_ = redisConnect("127.0.0.1", 6379); // 负责订阅消息的上下文连接
    if (subscribe_context_ == nullptr)
    {
        std::cerr << "connent redis failed" << std::endl;
        return false;
    }
    // 启动独立线程监听并接收订阅通道中的消息,并上报给server
    std::thread t([&]()
                  { observer_channel_msg(); });
    t.detach();
    std::cout << "connent redis success" << std::endl;
    return true;
}
bool Redis::publish(int channel, std::string msg) // 向redis指定通道channel发布消息
{
    // 发送发布消息命令函数
    redisReply *reply = (redisReply *)redisCommand(publish_context_, "publish %d %s", channel, msg.c_str());
    if (reply == nullptr)
    {
        std::cerr << "publish command failed" << std::endl;
        return false;
    }
    freeReplyObject(reply);
    return true;
}
bool Redis::subscribe(int channel) // 向redis指定通道subcribe订阅消息
{
    // 1.由于subcribe命令会造成线程阻塞等待通道里面发生消息,有消息才返回
    // 2.所有这个函数只负责发送命令到本地redis缓冲区,不阻塞接收redis server响应消息，通道的接收专门在独立线程observer_channel_msg中进行
    if (REDIS_ERR == redisAppendCommand(subscribe_context_, "subscribe %d", channel))
    {
        std::cerr << "subcribe command failed" << std::endl;
        return false;
    }
    // redisBufferWrite函数可以循环发送redisAppendCommand保存到本地redis缓冲区的消息到redis服务端上，发送完后会吧done置为1
    int done = 0;
    while (!done)
    {
        if (REDIS_ERR == redisBufferWrite(subscribe_context_, &done))
        {
            std::cerr << "subcribe command failed" << std::endl;
            return false;
        }
    }
    return true;
}
bool Redis::unsubscribe(int channel) // 向redis指定通道unsubcribe取消订阅消息
{
    // 与subcribe一样只改变一下命令就行
    if (REDIS_ERR == redisAppendCommand(subscribe_context_, "unsubscribe %d", channel))
    {
        std::cerr << "unsubcribe command failed" << std::endl;
        return false;
    }
    int done = 0;
    while (!done)
    {
        if (REDIS_ERR == redisBufferWrite(subscribe_context_, &done))
        {
            std::cerr << "unsubcribe command failed" << std::endl;
            return false;
        }
    }
    return true;
}
void Redis::observer_channel_msg() // 在独立子线程中接收订阅通道中的消息防止因订阅为阻塞执行导致主线程无法继续运行
{
    redisReply *reply = nullptr;
    //redisGetReply函数接收服务端响应，成功接收到时将响应内容保存在reply中,并返回0
    while (REDIS_OK == redisGetReply(subscribe_context_, (void **)&reply))
    {
        //订阅收到的消息都是一个带三元素的数组
        //1:"message"
        //2:int channel
        //3:收到的响应消息
        if((reply->type != REDIS_REPLY_ARRAY || reply->elements != 3))
        {
            std::cerr << "Unexpected reply type/elements: type=" << reply->type 
                      << ", elements=" << reply->elements << std::endl;
            freeReplyObject(reply);
            continue;
        }
        if (reply != nullptr && reply->element[2] != nullptr && reply->element[2]->str != nullptr&&reply->element[1] != nullptr && reply->element[1]->str != nullptr)
        {
            //回调操作，给业务层server上报通道上发生的消息
            notify_handler_(atoi(reply->element[1]->str),reply->element[2]->str);
        }
        freeReplyObject(reply);
    }
    std::cerr<<"observer_channel_quit"<<std::endl;
}
void Redis::init_notify_handler(std::function<void(int, std::string)> fn) // 初始化业务层server注册上报的回调函数
{
    this->notify_handler_ = fn;
}