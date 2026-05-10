#pragma once
#include <muduo/net/TcpServer.h>
#include <nlohmann/json.hpp>
#include <string>
#include "usermodel.hpp"
#include "offlineusermodel.hpp"
#include "friends.hpp"
#include "groupmodel.hpp"
#include "hiredis.hpp"
#include <unordered_map>
#include <functional>
#include <mutex>
using namespace muduo;
using namespace muduo::net;
using namespace nlohmann;
using fun = std::function<void(const TcpConnectionPtr &, json &, Timestamp)>;

class chatservice//主服务类
{
public:
    static chatservice *init();                                               // 返回单例
    void logto(const TcpConnectionPtr &conn, json &js, Timestamp time);       // 登录
    void regt(const TcpConnectionPtr &conn, json &js, Timestamp time);        // 注册
    void clientclose(const TcpConnectionPtr &conn,int id);                           // 客户端连接断开执行
    void reset();                                                             // 服务端异常退出,将在线全变为离线
    void chat(const TcpConnectionPtr &conn, json &js, Timestamp time);        // 在线对话聊天
    void addfriend(const TcpConnectionPtr &conn, json &js, Timestamp time);   // 添加好友
    void creategroup(const TcpConnectionPtr &conn, json &js, Timestamp time); // 创建群
    void addgroup(const TcpConnectionPtr &conn, json &js, Timestamp time);    // user加入群
    void groupchat(const TcpConnectionPtr &conn, json &js, Timestamp time);   // 群聊
    void offline(const TcpConnectionPtr &conn, json &js, Timestamp time);     // 用户下线
    void remove(const TcpConnectionPtr &conn, json &js, Timestamp time);      // user移除群
    int get_id(const TcpConnectionPtr &conn);                                 // 获取当前是那个用户id连接此tcp
    fun headlrback(int msgid);                                                // 返回对应的执行函数
    void updateActiveTime(const TcpConnectionPtr &conn);					//更新user的时间戳
    void checkTimeout();													//定时器
    void redishandler(int channel, std::string msg);                          // redis上报回调函数
private:
    chatservice();                                    // 单例模式
    std::unordered_map<int, fun> services_;           // id和对应的服务函数
    UserModel usermodel_;                             // 管理用户表，user表
    Offmodel offmodel_;                               // 管理用户的离线消息，offusermodel表
    Friend friend_;                                   // 管理用户好友列表，friends表
    Groupmodel groupmodel_;                           // 管理群列表，allgroup和groupuser表
    Redis redis_;                                     // redis队列
    std::unordered_map<int, TcpConnectionPtr> users_; // 保存管理在线用户,因为一个tcpconnentptr对应一个连接，有连接就说明在线
    std::unordered_map<TcpConnectionPtr,time_t> timeout_;//管理每个在线user上次活跃时间
    std::mutex mtx_;
    std::unordered_map<TcpConnectionPtr, int> idmap_; // 表示那些用户id在用,用map好实现多用户在线查询要的用户id
};