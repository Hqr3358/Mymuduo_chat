#include "chatserver.hpp"
#include"chatservice.hpp"
#include<signal.h>
#include<iostream>
void resetHandler(int)
{
    chatservice::init()->reset();
    exit(0);
}
int main(int argv,char** argc)
{
    if(argv!=3)
    {
        std::cerr<<"参数不对"<<std::endl;
        exit(0);
    }
    signal(SIGINT,resetHandler);//服务端异常退出,将在线全变为离线
    EventLoop loop;
    InetAddress addr(argc[1], atoi(argc[2]));
    chatserver server(&loop, addr, "chatserver");
    server.start();
    loop.loop();
    return 0;
}