#include "chatclient.hpp"
#include <cerrno>
#include <nlohmann/json.hpp>
#include "public.hpp"
#include <iostream>
#include <pthread.h>
#include <sstream>
#include <chrono>
#include <ctime>
#include <iomanip>
#include <semaphore.h>
#include <sys/socket.h>
#include <thread>
using nlohmann::json;
bool flag;//由于进程退出标志
bool online;//服务端断开标志
bool quit;//是否下线
sem_t rwsem;
std::string chatclient::get_time() // 获取当前时间
{
    auto now = std::chrono::system_clock::now();
    // 转换为 time_t
    std::time_t now_time_t = std::chrono::system_clock::to_time_t(now);
    // 格式化输出
    std::ostringstream ss;
    ss << std::put_time(std::localtime(&now_time_t), "%Y-%m-%d %H:%M:%S");
    return ss.str();
}
chatclient::chatclient(std::string &ip, uint16_t port) // 连接服务端并获取登录
{
    sockfd_ = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd_ < 0)
    {
        std::cerr << "socket fail" << std::endl;
        exit(-1);
    }
    struct sockaddr_in serveraddr;
    memset(&serveraddr, '0', sizeof(serveraddr));
    serveraddr.sin_family = AF_INET;
    serveraddr.sin_port = htons(port);
    serveraddr.sin_addr.s_addr = inet_addr(ip.c_str());
    if(connect(sockfd_, (const sockaddr *)&serveraddr, sizeof(serveraddr)) < 0)
    {
        std::cerr << "connent fail" << std::endl;
        exit(-1);
    }
    sem_init(&rwsem, 0, 0);   // 初始化信号量
    std::thread t(&chatclient::recvmsg,this); // 子线程用于接收消息
	std::thread t1(&chatclient::heartbeat,this);//心跳机制
    t.detach();
    t1.detach();
    quit=true;
    while (quit)
    {
        std::cout << "================" << std::endl;
        std::cout << "登录:1" << std::endl;
        std::cout << "注册:2" << std::endl;
        std::cout << "退出:3" << std::endl;
        std::cout << "================" << std::endl;
        std::cout << "选择:";
        int choose;
        while (!(std::cin >> choose))
        {
            std::cout << "请重新输入有效的id值:" << std::endl;
            std::cin.clear();                                                   // 清除错误状态
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n'); // 清空输入缓冲区
        }
        std::cin.get();
        switch (choose)
        {
        case 1:
            logto(sockfd_);   // 登录
            sem_wait(&rwsem); // 等待子线程接收到登录信息 
            if(flag)
            {
                start(sockfd_); // 开始通讯
            }
            break;
        case 2:
            regt(sockfd_);    // 注册
            sem_wait(&rwsem); // 等待子线程接收到注册信息
            break;
        case 3:
            quit=false;
            close(sockfd_);
            break;
        default:
            std::cout << "选择错误,重新选择" << std::endl;
            continue;
        }
    }
}
void chatclient::recvmsg() // 子线程用于接收消息
{
    int n;
    char buff[4096];
    while (true)
    {
        memset(buff, '\0', sizeof(buff));
        n = recv(sockfd_, buff, sizeof(buff) - 1, 0); // 接收响应
        if (n > 0)
        {
            buff[n] = '\0';
            json response;
            response = json::parse(buff);
            std::cout << std::endl;
            std::cout << response.dump() << std::endl; // 显示服务端响应的内容
            if(response["msgid"]=="LOGTO")
            {
                flag = true;
                online=true;
            }
            else
            {
                std::cout << "选择:" << std::flush;
            }
            sem_post(&rwsem); // 通知主线程接收成功
        }
        else if (n == 0)
        {
            std::cout << std::endl;
            flag=false;
            online=false;
            quit=false;
            close(sockfd_);
            break;
        }
        else
        {
            std::cerr << "接收服务端响应失败" << std::endl;
            break;
        }
    }
}
void chatclient::heartbeat()
{
    while(true)
    {
        sleep(15);
        json js;
        js["msg"]="ping";
        std::string sendmsg=js.dump();
        int n=send(sockfd_,sendmsg.c_str(),sendmsg.size(),MSG_NOSIGNAL);
        if(n<0)
        {
            if(errno==EPIPE)
            {
                break;
            }
        }
    }
}
void chatclient::start(int sockfd) // 开始通讯
{
    std::cout << "<================>" << std::endl;
    std::cout << "聊天:3" << std::endl;
    std::cout << "加好友:4" << std::endl;
    std::cout << "创建群:5" << std::endl;
    std::cout << "加入群:6" << std::endl;
    std::cout << "群聊:7" << std::endl;
    std::cout << "退出群:8" << std::endl;
    std::cout << "下线:9" << std::endl;
    std::cout << "<================>" << std::endl;
    std::cout << "选择:";
    while(online)
    {
        int choose;
        while (!(std::cin >> choose))
        {
            std::cout << "请重新输入有效的id值:" << std::endl;
            std::cin.clear();                                                   // 清除错误状态
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n'); // 清空输入缓冲区
        }
        if(!online)
        {
            std::cout << "服务端与你断开请重新登录" << std::endl;
            break;
        }
        std::cin.get();
        switch (choose)
        {
        case 3:
            chat(sockfd); // 聊天
            break;
        case 4:
            friends(sockfd); // 加好友
            break;
        case 5:
            creategroup(sockfd); // 创建群
            break;
        case 6:
            addgroup(sockfd); // 加入群
            break;
        case 7:
            groupchat(sockfd); // 群聊天
            break;
        case 8:
            removegroup(sockfd); // 退出群
            break;
        case 9:
            offline(sockfd); //用户下线
            online=false;
            break;
        default:
            std::cout << "选择错误,重新选择" << std::endl;
            break;
        }
    }
}
void chatclient::logto(int sockfd) // 登录
{
    json js;
    int id;
    std::cout << "登录id:";
    while (!(std::cin >> id))
    {
        std::cout << "请重新输入有效的id值:" << std::endl;
        std::cin.clear();                                                   // 清除错误状态
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n'); // 清空输入缓冲区
    }
    std::cin.get();
    char passwd[64];
    std::cout << "登录密码:";
    std::cin.getline(passwd, sizeof(passwd));
    js["msgid"] = LOGTO;
    js["id"] = id;
    js["passwd"] = passwd;
    std::string sendmsg = js.dump();
    int n = send(sockfd, sendmsg.c_str(), sendmsg.size(), 0); // 发送登录请求
    if (n < 0)
    {
        std::cerr << "发送登录消息失败" << std::endl;
        exit(-1);
    }
}
void chatclient::regt(int sockfd) // 注册
{
    json js;
    std::cout << "注册姓名:";
    char name[64];
    std::cin.getline(name, sizeof(name));
    std::cout << "注册密码:";
    char passwd[64];
    std::cin.getline(passwd, sizeof(passwd));
    js["msgid"] = REGT;
    js["name"] = name;
    js["passwd"] = passwd;
    std::string sendmsg = js.dump();
    int n = send(sockfd, sendmsg.c_str(), sendmsg.size(), 0); // 发送注册请求
    if (n < 0)
    {
        std::cerr << "发送注册消息失败" << std::endl;
        exit(-1);
    }
}
void chatclient::chat(int sockfd) // 单人聊天

{
    json js;
    std::cout << "聊天的对方id:";
    int to_id;
    while (!(std::cin >> to_id))
    {
        std::cout << "请重新输入有效的id值:" << std::endl;
        std::cin.clear();                                                   // 清除错误状态
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n'); // 清空输入缓冲区
    }
    std::cin.get();
    std::cout << "给对方发送的消息:";
    char message[64];
    std::cin.getline(message, sizeof(message));
    js["msgid"] = CHAT;
    js["to_id"] = to_id;
    js["msg"] = message;
    js["time"] = get_time();
    std::string sendmsg = js.dump();
    int n = send(sockfd, sendmsg.c_str(), sendmsg.size(), 0); // 发送聊天消息
    if (n < 0)
    {
        std::cerr << "发送聊天消息失败" << std::endl;
        exit(-1);
    }
}
void chatclient::friends(int sockfd) // 好友
{
    json js;
    std::cout << "加好友对方的id:";
    int to_id;
    while (!(std::cin >> to_id))
    {
        std::cout << "请重新输入有效的id值:" << std::endl;
        std::cin.clear();                                                   // 清除错误状态
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n'); // 清空输入缓冲区
    }
    std::cin.get();
    js["msgid"] = ADDFRIEND;
    js["add_id"] = to_id;
    std::string sendmsg = js.dump();
    int n = send(sockfd, sendmsg.c_str(), sendmsg.size(), 0); // 发送加好友消息
    if (n < 0)
    {
        std::cerr << "发送加好友消息失败" << std::endl;
        exit(-1);
    }
}
void chatclient::creategroup(int sockfd) // 创建群
{
    json js;
    std::cout << "创建群的名称:";
    char name[64];
    std::cin.getline(name, sizeof(name));
    std::cout << "创建群的描述:";
    char desc[64];
    std::cin.getline(desc, sizeof(desc));
    js["msgid"] = CREATEGROUP;
    js["name"] = name;
    js["desc"] = desc;
    std::string sendmsg = js.dump();
    int n = send(sockfd, sendmsg.c_str(), sendmsg.size(), 0); // 发送创建群消息
    if (n < 0)
    {
        std::cerr << "发送创建群消息失败" << std::endl;
        exit(-1);
    }
}
void chatclient::addgroup(int sockfd) // 加入群
{
    json js;
    std::cout << "要加入群的id:";
    int to_id;
    while (!(std::cin >> to_id))
    {
        std::cout << "请重新输入有效的id值:" << std::endl;
        std::cin.clear();                                                   // 清除错误状态
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n'); // 清空输入缓冲区
    }
    std::cin.get();
    js["msgid"] = INSERTGROUP;
    js["groupid"] = to_id;
    std::string sendmsg = js.dump();
    int n = send(sockfd, sendmsg.c_str(), sendmsg.size(), 0); // 发送加入群的消息
    if (n < 0)
    {
        std::cerr << "发送加入群消息失败" << std::endl;
        exit(-1);
    }
}
void chatclient::groupchat(int sockfd) // 群聊天
{
    json js;
    std::cout << "群聊的id:";
    int to_id;
    while (!(std::cin >> to_id))
    {
        std::cout << "请重新输入有效的id值:" << std::endl;
        std::cin.clear();                                                   // 清除错误状态
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n'); // 清空输入缓冲区
    }
    std::cin.get();
    char msg[1024];
    std::cout << "你要输出的群消息:";
    std::cin.getline(msg, sizeof(msg));
    js["msgid"] = GROUPCHAT;
    js["groupid"] = to_id;
    js["msg"] = msg;
    js["time"] = get_time();
    std::string sendmsg = js.dump();
    int n = send(sockfd, sendmsg.c_str(), sendmsg.size(), 0); // 发送群聊的消息
    if (n < 0)
    {
        std::cerr << "发送群聊消息失败" << std::endl;
        exit(-1);
    }
}
void chatclient::offline(int sockfd) //用户下线
{
    json js;
    js["msgid"] = OFFLINE;
    std::string sendmsg = js.dump();
    int n = send(sockfd, sendmsg.c_str(), sendmsg.size(), 0); // 发送下线的消息
    if (n < 0)
    {
        std::cerr << "发送下线消息失败" << std::endl;
        exit(-1);
    }
    quit=false;
}

void chatclient::removegroup(int sockfd) // 退出群
{
    json js;
    std::cout << "要退出的群id:";
    int to_id;
    while (!(std::cin >> to_id))
    {
        std::cout << "请重新输入有效的id值:" << std::endl;
        std::cin.clear();                                                   // 清除错误状态
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n'); // 清空输入缓冲区
    }
    std::cin.get();
    js["msgid"] = REMOVEUSER;
    js["removegroupid"] = to_id;
    std::string sendmsg = js.dump();
    int n = send(sockfd, sendmsg.c_str(), sendmsg.size(), 0); // 发送退出群的消息
    if (n < 0)
    {
        std::cerr << "发送退出群消息失败" << std::endl;
        exit(-1);
    }
}
