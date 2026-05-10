#include <unistd.h>
#include <string>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
class chatclient
{
public:
    chatclient(std::string &ip, uint16_t port); // 连接服务端
    void recvmsg();            					// 子线程用于接收消息
    void heartbeat();							//心跳机制
    void start(int sockfd);                     // 开始通讯
    std::string get_time();                     // 获取当前时间
    void logto(int sockfd);                     // 登录
    void regt(int sockfd);                      // 注册
    void chat(int sockfd);                      // 单人聊天
    void friends(int sockfd);                   // 好友
    void creategroup(int sockfd);               // 创建群
    void addgroup(int sockfd);                  // 加入群
    void offline(int sockfd);                   // 用户下线
    void groupchat(int sockfd);                 // 群聊天
    void removegroup(int sockfd);               // 退出群
private:
    int sockfd_;
};