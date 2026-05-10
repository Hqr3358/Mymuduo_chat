#include"chatclient.hpp"
#include<iostream>
int main(int argc,char** argv)
{
    if(argc!=3)//参数不对
    {
        std::cout<<"参数不对"<<std::endl;
        exit(0);
    }
    //获得服务端ip和端口
    std::string ip=argv[1];
    uint16_t port=atoi(argv[2]);
    chatclient client(ip,port);

    return 0;
}