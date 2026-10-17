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

// 主业务类：单例，统一处理 注册/登录/单聊/加好友/建群/加群/群聊/退群/下线
class chatservice
{
public:
    static chatservice *init();
    void logto(const TcpConnectionPtr &conn, json &js, Timestamp time);
    void regt(const TcpConnectionPtr &conn, json &js, Timestamp time);
    void clientclose(const TcpConnectionPtr &conn, int id);  // 连接断开清理
    void resetstate();                                       // 进程退出时把 online 落库为 offline
    bool want_quit();                                        // 信号置位后 loop 周期检查
    void chat(const TcpConnectionPtr &conn, json &js, Timestamp time);
    void addfriend(const TcpConnectionPtr &conn, json &js, Timestamp time);
    void creategroup(const TcpConnectionPtr &conn, json &js, Timestamp time);
    void addgroup(const TcpConnectionPtr &conn, json &js, Timestamp time);
    void groupchat(const TcpConnectionPtr &conn, json &js, Timestamp time);
    void offline(const TcpConnectionPtr &conn, json &js, Timestamp time);
    void remove(const TcpConnectionPtr &conn, json &js, Timestamp time);
    int  get_id(const TcpConnectionPtr &conn);
    fun  headlrback(const std::string& msgid);
    void updateActiveTime(const TcpConnectionPtr &conn);
    void checkTimeout();
    void redishandler(int channel, std::string msg);          // redis 跨机消息回调

private:
    chatservice();
    chatservice(const chatservice &) = delete;
    chatservice &operator=(const chatservice &) = delete;

    std::unordered_map<std::string, fun> services_;            // msgid -> 业务函数
    UserModel      usermodel_;
    Offmodel       offmodel_;
    Friend         friend_;
    Groupmodel     groupmodel_;
    Redis          redis_;

    // 在线状态（受 mtx_ 保护）
    std::unordered_map<int, TcpConnectionPtr>          users_;    // userid -> conn
    std::unordered_map<TcpConnectionPtr, int>          idmap_;    // conn -> userid
    std::unordered_map<TcpConnectionPtr, time_t>       timeout_;   // conn -> last active
    std::mutex mtx_;
    std::atomic<bool> quit_{false};
};
