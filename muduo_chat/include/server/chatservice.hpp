#pragma once
#include <muduo/net/TcpServer.h>
#include <muduo/net/EventLoop.h>
#include <nlohmann/json.hpp>
#include <string>
#include <vector>
#include "usermodel.hpp"
#include "offlineusermodel.hpp"
#include "friends.hpp"
#include "groupmodel.hpp"
#include "hiredis.hpp"
#include <unordered_map>
#include <functional>
#include <mutex>
#include <atomic>

using namespace muduo;
using namespace muduo::net;
using namespace nlohmann;
using fun = std::function<void(const TcpConnectionPtr &, json &, Timestamp)>;

/**
 * @brief 聊天服务器核心业务类（单例）
 * @details 统一处理 注册/登录/单聊/加好友/建群/加群/群聊/退群/主动下线，
 *          维护在线用户表，对接 MySQL 持久化与 Redis 跨机消息路由
 */
class chatservice
{
public:
    static chatservice *init();
    void logto(const TcpConnectionPtr &conn, json &js, Timestamp time);       // 登录
    void regt(const TcpConnectionPtr &conn, json &js, Timestamp time);         // 注册
    void clientclose(const TcpConnectionPtr &conn, int id);                    // 连接断开统一清理
    void resetstate();                                                         // 进程退出时把 online 落库为 offline
    bool want_quit();                                                          // 信号置位后供 loop 周期检查
    void chat(const TcpConnectionPtr &conn, json &js, Timestamp time);          // 单聊
    void addfriend(const TcpConnectionPtr &conn, json &js, Timestamp time);    // 添加好友
    void creategroup(const TcpConnectionPtr &conn, json &js, Timestamp time);  // 创建群
    void addgroup(const TcpConnectionPtr &conn, json &js, Timestamp time);     // 加入群
    void groupchat(const TcpConnectionPtr &conn, json &js, Timestamp time);    // 群聊
    void offline(const TcpConnectionPtr &conn, json &js, Timestamp time);       // 主动下线
    void remove(const TcpConnectionPtr &conn, json &js, Timestamp time);       // 退出群
    int  get_id(const TcpConnectionPtr &conn);                                 // 通过 conn 反查 userid
    fun  headlrback(const std::string& msgid);                                 // 根据 msgid 路由到对应业务函数
    void updateActiveTime(const TcpConnectionPtr &conn);                       // 刷新连接活跃时间
    void checkTimeout();                                                       // 周期踢掉空闲超时连接
    void redishandler(int channel, std::string msg);                            // redis 跨机消息到达回调

private:
    chatservice();
    chatservice(const chatservice &) = delete;
    chatservice &operator=(const chatservice &) = delete;

    std::unordered_map<std::string, fun> services_;          // msgid -> 业务处理函数
    UserModel      usermodel_;                               // user 表操作
    Offmodel       offmodel_;                               // 离线消息表操作
    Friend         friend_;                                  // friends 表操作
    Groupmodel     groupmodel_;                              // allgroup / groupuser 表操作
    Redis          redis_;                                   // 跨机消息发布订阅

    std::unordered_map<int, TcpConnectionPtr>          users_;   // userid -> conn，本 server 在线用户
    std::unordered_map<TcpConnectionPtr, int>          idmap_;   // conn -> userid
    std::unordered_map<TcpConnectionPtr, time_t>       timeout_; // conn -> 上次活跃时间
    std::mutex mtx_;                                             // 保护上面三张表
    std::atomic<bool> quit_{false};
};
