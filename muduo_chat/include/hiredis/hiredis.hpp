#include<hiredis/hiredis.h>
#include<functional>
#include<string>
class Redis
{
public:
    Redis();
    ~Redis();
    bool connect();//连接redis服务端
    bool publish(int channel,std::string msg);//向redis指定通道channel发布消息
    bool subscribe(int channel);//向redis指定通道subcribe订阅消息
    bool unsubscribe(int channel);//向redis指定通道unsubcribe取消订阅消息
    void observer_channel_msg();//在独立线程中接收订阅通道中的消息防止因订阅为阻塞执行导致redis无法正常运行
    void init_notify_handler(std::function<void(int,std::string)> fn);// 初始化业务层server注册上报的回调函数
private:
    redisContext* publish_context_;//发布上下文
    redisContext* subscribe_context_;//订阅上下文
    std::function<void(int,std::string)> notify_handler_;//回调操作，给业务层server上报通道上发生的消息->id+msg
};