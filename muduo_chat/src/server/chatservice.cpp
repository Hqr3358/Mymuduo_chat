#include "chatservice.hpp"
#include "public.hpp"
#include "user.hpp"
#include "config.hpp"
#include <muduo/base/Logging.h>
#include <muduo/net/Callbacks.h>
#include <mutex>
#include <vector>
#include <utility>
#include <iostream>

chatservice *chatservice::init()   // 返回业务单例
{
    static chatservice ser;
    return &ser;
}

chatservice::chatservice()   // 构造：注册消息处理器 + 连接 redis + 注册跨机消息回调
{
    // 优化：msgid 统一用字符串做 key，避免数字/字符串混用导致客户端 json 比较抛异常
    services_[REGT]        = std::bind(&chatservice::regt,        this, ::_1, ::_2, ::_3);
    services_[LOGTO]       = std::bind(&chatservice::logto,       this, ::_1, ::_2, ::_3);
    services_[CHAT]        = std::bind(&chatservice::chat,        this, ::_1, ::_2, ::_3);
    services_[ADDFRIEND]   = std::bind(&chatservice::addfriend,   this, ::_1, ::_2, ::_3);
    services_[CREATEGROUP] = std::bind(&chatservice::creategroup, this, ::_1, ::_2, ::_3);
    services_[INSERTGROUP] = std::bind(&chatservice::addgroup,    this, ::_1, ::_2, ::_3);
    services_[GROUPCHAT]   = std::bind(&chatservice::groupchat,  this, ::_1, ::_2, ::_3);
    services_[OFFLINE]     = std::bind(&chatservice::offline,     this, ::_1, ::_2, ::_3);
    services_[REMOVEUSER]  = std::bind(&chatservice::remove,      this, ::_1, ::_2, ::_3);

    if (redis_.connect())
    {
        redis_.init_notify_handler(std::bind(&chatservice::redishandler, this, ::_1, ::_2));
    }
}

bool chatservice::want_quit() { return quit_.load(); }   // 供 main 线程周期轮询退出信号

void chatservice::clientclose(const TcpConnectionPtr &conn, int id)   // 连接断开统一清理：踢 map、落 DB、退 redis
{
    {
        std::lock_guard<std::mutex> lock(mtx_);
        idmap_.erase(conn);
        // 优化：原来断连后 timeout_ 残留垃圾，这里一并清掉
        timeout_.erase(conn);
        if (id != 0) users_.erase(id);
    }
    if (id != 0)
    {
        User user;
        user.setid(id);
        user.setstate("offonline");
        usermodel_.update(user);
        redis_.unsubscribe(id);
    }
    std::cout << "client close, id=" << id << std::endl;
}

void chatservice::resetstate()   // 进程退出时把 online 批量置为 offline
{
    usermodel_.resetstate();
}

void chatservice::logto(const TcpConnectionPtr &conn, json &js, Timestamp)   // 登录
{
    int id = js["id"].get<int>();
    std::string passwd = js["passwd"].get<std::string>();
    User user = usermodel_.query(id);

    if (user.getpassword() == passwd)
    {
        if (user.getstate() == "online")
        {
            json response = {{"msgid", ERRNO}, {"errno", -1}, {"msg", "你已经在线了"}};
            conn->send(response.dump());
            return;
        }
        user.setstate("online");
        {
            std::lock_guard<std::mutex> lock(mtx_);
            users_[id] = conn;
            idmap_[conn] = id;
            timeout_[conn] = time(nullptr);
        }
        usermodel_.update(user);
        redis_.subscribe(id);

        json response;
        response["msgid"] = LOGTO;
        response["msg"]   = "登录成功";

        std::vector<std::string> friends = friend_.query(id);
        std::vector<Group> groups = groupmodel_.querygroup(id);
        std::vector<std::string> offmsg = offmodel_.query(id);

        response["offlinemsg"] = offmsg.empty() ? json("null") : json(offmsg);
        response["friends"]    = friends;

        // 优化：原来 vec 在 for 循环内部定义，导致 groups 只保留最后一个群
        std::vector<std::string> groupList;
        for (auto &g : groups)
        {
            groupList.push_back("====id:" + std::to_string(g.getgroupid())
                              + " name:" + g.getgroupname()
                              + " desc:" + g.getgroupdesc() + "====");
        }
        response["groups"] = groupList;

        conn->send(response.dump());
        offmodel_.remove(id);   // 已推送的离线消息清除
    }
    else
    {
        json response = {{"msgid", ERRNO}, {"errno", -1}, {"msg", "id 或密码错误"}};
        conn->send(response.dump());
    }
}

void chatservice::regt(const TcpConnectionPtr &conn, json &js, Timestamp)   // 注册
{
    std::string name = js["name"].get<std::string>();
    std::string passwd = js["passwd"].get<std::string>();
    User user;
    user.setname(name);
    user.setpassword(passwd);
    if (usermodel_.insert(user))
    {
        json response = {{"msgid", REGT}, {"errno", 0},
                         {"id", user.getid()}, {"msg", "注册成功"}};
        conn->send(response.dump());
    }
    else
    {
        json response = {{"msgid", ERRNO}, {"errno", -1},
                         {"errmsg", "注册失败，用户名可能已存在"}};
        conn->send(response.dump());
    }
}

void chatservice::chat(const TcpConnectionPtr &conn, json &js, Timestamp)   // 单聊
{
    int from_id = get_id(conn);
    if (from_id == 0)
    {
        conn->send(json({{"msgid", ERRNO}, {"errno", -1}, {"errmsg", "请先登录"}}).dump());
        return;
    }
    int to_id    = js["to_id"].get<int>();
    std::string msg = js["msg"].get<std::string>();

    // 优化：锁内只取 to_conn，锁外做 DB 查询，缩短临界区
    TcpConnectionPtr to_conn;
    {
        std::lock_guard<std::mutex> lock(mtx_);
        auto it = users_.find(to_id);
        if (it != users_.end()) to_conn = it->second;
    }

    User to_user = usermodel_.query(to_id);
    if (to_user.getid() == 0)   // 对方根本不存在
    {
        conn->send(json({{"msgid", ERRNO}, {"errno", -1}, {"errmsg", "该用户不存在"}}).dump());
        return;
    }

    if (to_conn)   // 对方在本 server 在线
    {
        json result = {{"msgid", CHAT}, {"from_id", from_id}, {"message", msg}};
        to_conn->send(result.dump());
        conn->send(json({{"msgid", CHAT}, {"message", "发送成功"}}).dump());
    }
    else if (to_user.getstate() == "online")   // 优化：对方在别的 server 在线，走 redis 发布跨机路由
    {
        json result = {{"msgid", CHAT}, {"from_id", from_id}, {"message", msg}};
        redis_.publish(to_id, result.dump());
        conn->send(json({{"msgid", CHAT}, {"message", "发送成功"}}).dump());
    }
    else   // 对方离线，存离线消息
    {
        std::string stored = "[" + js["time"].get<std::string>() + "] user"
                           + std::to_string(from_id) + ": " + msg;
        offmodel_.insert(to_id, stored);
        conn->send(json({{"msgid", CHAT}, {"errno", 0},
                         {"message", "对方不在线，已为你保存消息"}}).dump());
    }
}

void chatservice::addfriend(const TcpConnectionPtr &conn, json &js, Timestamp)   // 添加好友
{
    int id = get_id(conn);
    if (id == 0)
    {
        conn->send(json({{"msgid", ERRNO}, {"errno", -1}, {"errmsg", "请先登录"}}).dump());
        return;
    }
    int add_id = js["add_id"].get<int>();
    if (friend_.insert(id, add_id))
    {
        conn->send(json({{"msgid", ADDFRIEND},
                         {"message", "成功添加好友->" + std::to_string(add_id)}}).dump());
    }
    else
    {
        conn->send(json({{"msgid", ERRNO}, {"errno", -1}, {"message", "添加好友失败"}}).dump());
    }
}

void chatservice::creategroup(const TcpConnectionPtr &conn, json &js, Timestamp)   // 创建群
{
    int id = get_id(conn);
    if (id == 0)
    {
        conn->send(json({{"msgid", ERRNO}, {"errno", -1}, {"errmsg", "请先登录"}}).dump());
        return;
    }
    Group group;
    group.setgroupname(js["name"].get<std::string>());
    group.setgroupdesc(js["desc"].get<std::string>());
    if (groupmodel_.creategroup(id, group))
    {
        conn->send(json({{"msgid", CREATEGROUP},
                         {"message", "成功创建群,id->" + std::to_string(group.getgroupid())}}).dump());
    }
    else
    {
        conn->send(json({{"msgid", ERRNO}, {"errno", -1}, {"message", "创建群失败"}}).dump());
    }
}

void chatservice::addgroup(const TcpConnectionPtr &conn, json &js, Timestamp)   // 加入群
{
    int userid = get_id(conn);
    if (userid == 0)
    {
        conn->send(json({{"msgid", ERRNO}, {"errno", -1}, {"errmsg", "请先登录"}}).dump());
        return;
    }
    int groupid = js["groupid"].get<int>();
    if (groupmodel_.insertuser(groupid, userid))
    {
        conn->send(json({{"msgid", INSERTGROUP},
                         {"message", "成功加入群,id->" + std::to_string(groupid)}}).dump());
    }
    else
    {
        conn->send(json({{"msgid", ERRNO}, {"errno", -1},
                         {"message", "加入群" + std::to_string(groupid) + "失败，可能已加入"}}).dump());
    }
}

void chatservice::groupchat(const TcpConnectionPtr &conn, json &js, Timestamp)   // 群聊
{
    int userid = get_id(conn);
    if (userid == 0)
    {
        conn->send(json({{"msgid", ERRNO}, {"errno", -1}, {"errmsg", "请先登录"}}).dump());
        return;
    }
    int groupid = js["groupid"].get<int>();
    std::string to_msg = js["msg"].get<std::string>();
    std::string prefix = "群[" + std::to_string(groupid) + "] user"
                       + std::to_string(userid) + ": ";

    std::vector<Groupuser> members = groupmodel_.query(groupid, userid);
    int offlineCnt = 0;
    for (auto &m : members)
    {
        TcpConnectionPtr to_conn;
        {
            std::lock_guard<std::mutex> lock(mtx_);
            auto it = users_.find(m.getid());
            if (it != users_.end()) to_conn = it->second;
        }

        if (to_conn)    // 本 server 在线，直接发
        {
            json result = {{"msgid", GROUPCHAT}, {"groupid", groupid},
                           {"from_id", userid}, {"message", prefix + to_msg}};
            to_conn->send(result.dump());
        }
        else
        {
            User u = usermodel_.query(m.getid());
            if (u.getstate() == "online")   // 优化：别的 server 在线，redis 发布跨机路由
            {
                json result = {{"msgid", GROUPCHAT}, {"groupid", groupid},
                               {"from_id", userid}, {"message", prefix + to_msg}};
                redis_.publish(m.getid(), result.dump());
            }
            else   // 离线，存离线消息
            {
                std::string stored = "[" + js["time"].get<std::string>() + "] " + prefix + to_msg;
                offmodel_.insert(m.getid(), stored);
                ++offlineCnt;
            }
        }
    }
    // 优化：原来循环内每个组员都给发送方回一次"发送成功"，N 个组员回 N 次，改为统一回一次
    json resp = {{"msgid", GROUPCHAT}, {"message", "群消息已分发"}};
    if (offlineCnt > 0) resp["offline"] = offlineCnt;
    conn->send(resp.dump());
}

void chatservice::offline(const TcpConnectionPtr &conn, json &, Timestamp)   // 主动下线
{
    int id = get_id(conn);
    // 优化：原来 offline 只改 DB 不清 users_/idmap_，导致后续消息丢失，这里复用统一清理
    clientclose(conn, id);
    if (conn->connected()) conn->shutdown();
}

void chatservice::remove(const TcpConnectionPtr &conn, json &js, Timestamp)   // 退出群
{
    int userid = get_id(conn);
    if (userid == 0)
    {
        conn->send(json({{"msgid", ERRNO}, {"errno", -1}, {"errmsg", "请先登录"}}).dump());
        return;
    }
    int groupid = js["removegroupid"].get<int>();
    groupmodel_.remove(groupid, userid);
    conn->send(json({{"msgid", REMOVEUSER},
                     {"message", "成功退出群,id->" + std::to_string(groupid)}}).dump());
}

fun chatservice::headlrback(const std::string &msgid)   // 根据 msgid 路由到对应业务函数
{
    auto it = services_.find(msgid);
    if (it == services_.end())
    {
        return [=](const TcpConnectionPtr &, json &js, Timestamp)
        {
            LOG_ERROR << "unknown msgid: " << msgid;
        };
    }
    return it->second;
}

void chatservice::updateActiveTime(const TcpConnectionPtr &conn)   // 刷新该 conn 的活跃时间
{
    std::lock_guard<std::mutex> lock(mtx_);
    timeout_[conn] = time(nullptr);
}

void chatservice::checkTimeout()   // 周期任务：踢掉空闲超时连接
{
    time_t now = time(nullptr);
    int timeout = Config::instance().idle_timeout_sec;
    // 优化：锁内一次性收集要踢的连接，锁外再 shutdown，避免持锁做 close 回调；用 find 不用 operator[]，避免污染 map
    std::vector<std::pair<TcpConnectionPtr,int>> dead;
    {
        std::lock_guard<std::mutex> lock(mtx_);
        for (auto it = timeout_.begin(); it != timeout_.end();)
        {
            if (now - it->second > timeout)
            {
                int id = 0;
                auto idIt = idmap_.find(it->first);
                if (idIt != idmap_.end()) id = idIt->second;
                dead.push_back({it->first, id});
                it = timeout_.erase(it);
            }
            else ++it;
        }
    }
    for (auto &[conn, id] : dead)
    {
        // conn->shutdown 会触发 onConnectionCallback -> clientclose，那里会再做一次幂等清理
        if (conn->connected()) conn->shutdown();
    }
}

void chatservice::redishandler(int channel, std::string msg)   // redis 订阅线程收到跨机消息的回调
{
    TcpConnectionPtr conn;
    {
        std::lock_guard<std::mutex> lock(mtx_);
        auto it = users_.find(channel);
        if (it != users_.end()) conn = it->second;
    }
    if (conn)
    {
        // 优化：muduo 要求 send 必须在 conn 所属 loop 线程调用，这里用 runInLoop 投递，避免跨线程数据竞争
        conn->getLoop()->runInLoop([conn, msg]() {
            conn->send(msg);
        });
    }
    else
    {
        offmodel_.insert(channel, msg);   // 用户刚好离线，落离线表
    }
}

int chatservice::get_id(const TcpConnectionPtr &conn)   // 通过 conn 反查 userid，未登录返回 0
{
    std::lock_guard<std::mutex> lock(mtx_);
    auto it = idmap_.find(conn);
    return it != idmap_.end() ? it->second : 0;
}
