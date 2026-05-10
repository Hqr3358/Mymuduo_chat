#include "chatservice.hpp"
#include "public.hpp"
#include "user.hpp"
#include <muduo/base/Logging.h>
#include <muduo/net/Callbacks.h>
#include <mutex>
chatservice *chatservice::init() // 返回单例
{
    static chatservice ser;
    return &ser;
}
chatservice::chatservice() // 将unordered_map填充
{
    services_.insert({REGT, std::bind(&chatservice::regt, this, ::_1, ::_2, ::_3)});               // 将注册服务与对应id绑定
    services_.insert({LOGTO, std::bind(&chatservice::logto, this, ::_1, ::_2, ::_3)});             // 将登录服务与对应id绑定
    services_.insert({CHAT, std::bind(&chatservice::chat, this, ::_1, ::_2, ::_3)});               // 将聊天服务与对应id绑定
    services_.insert({ADDFRIEND, std::bind(&chatservice::addfriend, this, ::_1, ::_2, ::_3)});     // 添加好友
    services_.insert({CREATEGROUP, std::bind(&chatservice::creategroup, this, ::_1, ::_2, ::_3)}); // 创建群
    services_.insert({INSERTGROUP, std::bind(&chatservice::addgroup, this, ::_1, ::_2, ::_3)});    // user加入群
    services_.insert({GROUPCHAT, std::bind(&chatservice::groupchat, this, ::_1, ::_2, ::_3)});     // 群聊
    services_.insert({OFFLINE, std::bind(&chatservice::offline, this, ::_1, ::_2, ::_3)});         // 用户下线
    services_.insert({REMOVEUSER, std::bind(&chatservice::remove, this, ::_1, ::_2, ::_3)});       // user移除群
    if (redis_.connect())
    {
        redis_.init_notify_handler(std::bind(&chatservice::redishandler, this, ::_1, ::_2)); // 初始化redis上报回调函数
    }
}
void chatservice::clientclose(const TcpConnectionPtr &conn,int id) 
{
    User user;
    {
        std::lock_guard<std::mutex> lock(mtx_);
        idmap_.erase(conn);
       	users_.erase(id);
    }
        user.setid(id);
        if (user.getid() != 0)
        {
            user.setstate("offonline");
            usermodel_.update(user);
        }
        redis_.unsubscribe(user.getid()); // 取消redis上订阅通道
}
void chatservice::reset() // 服务端异常退出,将在线全变为离线
{
    usermodel_.resetstate();
}
void chatservice::logto(const TcpConnectionPtr &conn, json &js, Timestamp time) // 登录
{
    int id = js["id"].get<int>(); // 知道该用户在表中的id好用于登录
    std::string passwd = js["passwd"];
    User user = usermodel_.query(id);//在数据库表中获取该user的信息用于验证
    // std::cout<<"-------------"<<user.getpassword()<<"------"<<passwd<<std::endl;
    // 密码正确，登录成功
    if (user.getpassword() == passwd)
    {
        if (user.getstate() == "online")
        {
            json response;
            response["msgid"] = "ERRNO";
            response["errno"] = -1;
            response["msg"] = "你已经在线了";
            conn->send(response.dump());
            return;
        }
        user.setstate("online");//设置为登录状态
        {
            std::lock_guard<std::mutex> lock(mtx_);
            users_.insert({id, conn}); // 添加到在线map和查询map里
            idmap_.insert({conn, id});
        }
        usermodel_.update(user); // 更新用户状态
        redis_.subscribe(id);// 在redis中订阅一个channel(id)
        json response;           // 1.用户登录信息json
        response["msgid"] = "LOGTO";
        response["msg"] = "登录成功";
        std::vector<std::string> friends = friend_.query(id);      // 这里得到friends表里的该用户id的全部好友id
        std::vector<Group> groups = groupmodel_.querygroup(id);    // 这里得到userid中所有群
        std::vector<std::string> offlinemsg = offmodel_.query(id); // 这里得到offlinemodel表里对应id的所有离线时的消息
        if (!offlinemsg.empty())
        {
            response["offlinemsg"] = offlinemsg;
        }
        else
        {
            response["offlinemsg"] = "null";
        }
        if (!groups.empty())
        {
            for (auto &group : groups)
            {
                std::vector<std::string> vec;
                std::string groupquery = "====id:" + std::to_string(group.getgroupid()) + " name:" + group.getgroupname() + " desc:" + group.getgroupdesc() + "====";
                vec.push_back(groupquery);
                response["groups"] = vec;
            }
        }
        if (!friends.empty())
        {
            response["friends"] = friends;
        }
        conn->send(response.dump()); // 2.先发送用户登录信息
        offmodel_.remove(id);   // 清除已看过的离线消息
    }
    //不正确
    else
    {
        json response;
        response["msgid"] = "ERRNO";
        response["errno"] = -1;
        response["msg"] = "你的id或密码不对";
        conn->send(response.dump());
    }
}
void chatservice::regt(const TcpConnectionPtr &conn, json &js, Timestamp time) // 注册
{
    std::string name = js["name"];
    std::string passwd = js["passwd"];
    User user;
    user.setname(name);
    user.setpassword(passwd);
    bool flag = usermodel_.insert(user); // 在注册表中插入该用户
    if (flag)                            // 注册成功
    {
        json response;
        response["msgid"] = "REGT";
        response["errno"] = 0;
        response["id"] = user.getid();
        response["msg"] = "注册成功";
        conn->send(response.dump());
    }
    else // 注册失败
    {
        json response;
        response["msgid"] = "ERRNO";
        response["errno"] = -1;
        response["errmsg"] = "注册失败你可能已经注册过了";
        conn->send(response.dump());
    }
}
void chatservice::chat(const TcpConnectionPtr &conn, json &js, Timestamp time) // 在线对话聊天
{
    int id = this->get_id(conn); // 发送方的id
    if (id == 0)                 // 用户跳过了登录
    {
        json response;
        response["msgid"] = "ERRNO";
        response["errno"] = -1;
        response["errmsg"] = "请先登录";
        conn->send(response.dump());
        return;
    }
    TcpConnectionPtr to_conn = nullptr;      // 接收方的connptr
    int to_id = js["to_id"].get<int>();      // 接收方的id
    std::string msg = js["msg"];             // 发送给接收方的消息
    std::unique_lock<std::mutex> lock(mtx_); // 给map容器加锁
    auto it = users_.find(to_id);   // 从用户在线表里查接收方的id
    if (it != users_.end())
    {
        to_conn = it->second;
    }
    lock.unlock();
    User user = usermodel_.query(to_id); // 在数据库表中获取对方id的详细信息
    // 在自己的users在线表中判断
    // 用户不存在或离线或在其他server上在线时
    if (it == users_.end())
    {
        // 用户不存在
        if (user.getid() == 0)
        {
            json response;
            response["msgid"] = "ERRNO";
            response["errno"] = -1;
            response["errmsg"] = "该用户不存在";
            conn->send(response.dump());
        }
        // 用户不在该server上但在其他server上在线
        else if (user.getid() != 0 && user.getstate() == "online")
        {
            json result;
            result["msgid"] = "CHAT";
            result["to_id"] = id;
            result["message"] = msg.c_str();
            redis_.publish(to_id, result.dump()); // 发送给redis通道channel(to_id)
            json response;
            response["msgid"] = "CHAT";
            response["message"] = "发送成功";
            conn->send(response.dump());
            return;
        }
        // 用户存在,但不在线保存消息给上线看
        else
        {
            msg += "发送时间:" + js["time"].get<std::string>() + "-->to_id:" + std::to_string(id);
            if (offmodel_.insert(to_id, msg))
            {
                json response;
                response["msgid"] = "CHAT";
                response["errno"] = 0;
                response["message"] = "该用户不在线已为你保存消息";
                conn->send(response.dump());
            }
        }
    }
    // 用户在该server上且在线
    else
    {
        json result;
        result["msgid"] = "CHAT";
        result["to_id"] = id;
        result["message"] = msg.c_str();
        to_conn->send(result.dump());
        json response;
        response["msgid"] = "CHAT";
        response["message"] = "发送成功";
        conn->send(response.dump());
    }
}
void chatservice::addfriend(const TcpConnectionPtr &conn, json &js, Timestamp time) // 添加好友
{
    int id = this->get_id(conn);
    if (id == 0) // 用户跳过了登录
    {
        json response;
        response["msgid"] = "ERRNO";
        response["errno"] = -1;
        response["errmsg"] = "请先登录";
        conn->send(response.dump());
        return;
    }
    int add_id = js["add_id"].get<int>();
    if (friend_.insert(id, add_id)) // 成功添加好友
    {
        json response;
        response["msgid"] = "ADDFRIEND";
        std::string msg = "成功添加好友->" + std::to_string(add_id); // to_do?
        response["message"] = msg;
        conn->send(response.dump());
    }
    // 添加失败
    else
    {
        json response;
        response["msgid"] = "ERRNO";
        response["errno"] = -1;
        response["message"] = "添加好友失败";
        conn->send(response.dump());
    }
}
void chatservice::creategroup(const TcpConnectionPtr &conn, json &js, Timestamp time) // 创建群
{
    int id = this->get_id(conn);
    if (id == 0) // 用户跳过了登录
    {
        json response;
        response["msgid"] = "ERRNO";
        response["errno"] = -1;
        response["errmsg"] = "请先登录";
        conn->send(response.dump());
        return;
    }
    std::string name = js["name"];
    std::string desc = js["desc"];
    std::unique_lock<std::mutex> lock(mtx_); // 给map容器加锁
    auto it = idmap_.find(conn);
    lock.unlock();
    if (it != idmap_.end())
    {
        int userid = it->second;
        Group group;
        group.setgroupname(name);
        group.setgroupdesc(desc);
        // 创建群成功
        if (groupmodel_.creategroup(userid, group))
        {
            json response;
            response["msgid"] = "CREATEGROUP";
            std::string msg = "成功创建群,id->" + std::to_string(group.getgroupid());
            response["message"] = msg.c_str();
            conn->send(response.dump());
            return;
        }
    }
    else
    {
        json response;
        response["msgid"] = "ERRNO";
        response["errno"] = -1;
        response["message"] = "创建群失败";
        conn->send(response.dump());
    }
}
void chatservice::addgroup(const TcpConnectionPtr &conn, json &js, Timestamp time) // user加入群
{
    int id = this->get_id(conn);
    if (id == 0) // 用户跳过了登录
    {
        json response;
        response["msgid"] = "ERRNO";
        response["errno"] = -1;
        response["errmsg"] = "请先登录";
        conn->send(response.dump());
        return;
    }
    int groupid = js["groupid"].get<int>();
    int userid;
    std::unique_lock<std::mutex> lock(mtx_); // 给map容器加锁
    auto it = idmap_.find(conn);
    lock.unlock();
    if (it != idmap_.end())
    {
        userid = it->second;
    }
    if (groupmodel_.insertuser(groupid, userid))
    {
        json response;
        response["msgid"] = "INSERTGROUP";
        std::string msg = "成功加入群,id->" + std::to_string(groupid);
        response["message"] = msg.c_str();
        conn->send(response.dump());
        return;
    }
    else
    {
        json response;
        response["msgid"] = "ERRNO";
        response["errno"] = -1;
        std::string flag = "请查看自己是否已经加入过该群";
        response["message"] = "加入群id->" + std::to_string(groupid) + "失败" + flag;
        conn->send(response.dump());
    }
}
void chatservice::groupchat(const TcpConnectionPtr &conn, json &js, Timestamp time) // 群聊
{
    int id = this->get_id(conn);
    if (id == 0) // 用户跳过了登录
    {
        json response;
        response["msgid"] = "ERRNO";
        response["errno"] = -1;
        response["errmsg"] = "请先登录";
        conn->send(response.dump());
        return;
    }
    int groupid = js["groupid"];
    int userid = get_id(conn);
    TcpConnectionPtr to_conn = nullptr;
    std::vector<Groupuser> vec = groupmodel_.query(groupid, userid);//把当前userid加入的groupid中除自己外所有组员得到
    for (auto &groupuser : vec)
    {
        std::unique_lock<std::mutex> lock(mtx_); // 给map容器加锁
        auto it = users_.find(groupuser.getid());
        lock.unlock();
        // 发送给该server在线的群组员
        std::string to_msg = js["msg"];
        User user = usermodel_.query(groupuser.getid()); // 在user表中获取对方id的详细信息
        if (it != users_.end())
        {
            to_conn = it->second;
            json result;
            result["msgid"] = "GROUPCHAT";
            std::string group = "群聊id:";
            group += std::to_string(groupid) + " 群组id:";
            std::string msg = group + std::to_string(userid) + "-->" + to_msg;
            result["message"] = msg.c_str();
            to_conn->send(result.dump());
            json response;
            response["msgid"] = "GROUPCHAT";
            response["message"] = "发送成功";
            conn->send(response.dump());
        }
        // 用户不在该server上但在线
        else if (user.getstate() == "online")
        {
            json result;
            result["msgid"] = "GROUPCHAT";
            std::string group = "群聊id:";
            group += std::to_string(groupid) + " 群组id:";
            std::string msg = group + std::to_string(userid) + "-->" + to_msg;
            result["message"] = msg.c_str();
            redis_.publish(user.getid(), result.dump()); // 发送给redis通道channel(user.getid())
            json response;
            response["msgid"] = "GROUPCHAT";
            response["message"] = "发送成功";
            conn->send(response.dump());
        }
        // 群组员不在线时
        else
        {
            std::string group = "离线消息群聊id:";
            group += std::to_string(groupid) + "群组id:";
            std::string msg = js["time"].get<std::string>() + group + std::to_string(userid) + "-->" + to_msg;
            offmodel_.insert(groupuser.getid(), msg);
            json response;
            response["msg"] = "已发送给了离线群组员";
            conn->send(response.dump());
        }
    }
}
void chatservice::offline(const TcpConnectionPtr &conn, json &js, Timestamp time) // 用户下线
{
    int id = get_id(conn);
    usermodel_.offline(id);
    redis_.unsubscribe(id);
    if(conn->connected()) conn->shutdown();
}
void chatservice::remove(const TcpConnectionPtr &conn, json &js, Timestamp time) // user移出群
{
    int id = this->get_id(conn);
    if (id == 0) // 用户跳过了登录
    {
        json response;
        response["msgid"] = "ERRNO";
        response["errno"] = -1;
        response["errmsg"] = "请先登录";
        conn->send(response.dump());
        return;
    }
    int groupid = js["removegroupid"];
    int userid;
    std::unique_lock<std::mutex> lock(mtx_); // 给map容器加锁
    auto it = idmap_.find(conn);
    lock.unlock();
    if (it != idmap_.end())
    {
        userid = it->second;
    }
    groupmodel_.remove(groupid, userid);
    json response;
    response["msgid"] = "REMOVEUSER";
    std::string msg = "成功退出群,id->" + std::to_string(groupid);
    response["message"] = msg.c_str();
    conn->send(response.dump());
}
fun chatservice::headlrback(int msgid) // 返回对应的执行函数
{
    auto it = services_.find(msgid);
    if (it == services_.end())
    {
        return [=](const TcpConnectionPtr &, json &js, Timestamp)
        {
            LOG_ERROR << "services not find msgid:" << msgid;
        };
    }
    return services_[msgid];
}
void chatservice::updateActiveTime(const TcpConnectionPtr &conn)
{
    timeout_[conn]=time(nullptr);//若没创建直接创建并更新conn时间（只负责保存conn活跃时间）
}
void chatservice::checkTimeout()
{
    time_t now=time(nullptr);
    for(auto it=timeout_.begin();it!=timeout_.end();)
   	{
        if(now-it->second>30)
        {
            //保留一个ptr计数让后面的clientclose能够调用
            TcpConnectionPtr conn=it->first;
            int id=idmap_[it->first];
            it=timeout_.erase(it);
            conn->shutdown();	
        }
        else
        {
            it++;
        }
    }
    
}
void chatservice::redishandler(int channel, std::string msg) // redis上报回调函数
{
    std::unique_lock<std::mutex> lock(mtx_); // 给map容器加锁
    auto it = users_.find(channel);
    lock.unlock();
    if (it != users_.end())
    {
        json response;
        response = json::parse(msg.c_str());
        it->second->send(response.dump());
        return;
    }
    // 用户突然离线时
    offmodel_.insert(channel, msg);
}
int chatservice::get_id(const TcpConnectionPtr &conn) // 获取当前是那个用户id连接该tcp
{
    std::unique_lock<std::mutex> lock(mtx_); // 给map容器加锁
    //可优化
    auto it = idmap_.find(conn);
    lock.unlock();
    if (it != idmap_.end())
    {
        return it->second;
    }
    return 0;
}
