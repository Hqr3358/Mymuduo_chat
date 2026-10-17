#include "chatserver.hpp"
#include "chatservice.hpp"
#include <signal.h>
#include <atomic>
#include <iostream>

// 信号处理函数里只能调用 async-signal-safe 的函数：只置一个 atomic flag，
// 真正的清理动作（mysql 更新、退出 loop）放到 loop 线程里做。
static std::atomic<bool> g_quit{false};

static void resetHandler(int)
{
    g_quit.store(true);
}

int main(int argc, char **argv)   // 修复原 bug：原来写成 (int argv, char** argc)
{
    if (argc != 3)
    {
        std::cerr << "usage: " << argv[0] << " <ip> <port>" << std::endl;
        exit(1);
    }
    signal(SIGINT, resetHandler);
    signal(SIGTERM, resetHandler);

    EventLoop loop;
    InetAddress addr(argv[1], atoi(argv[2]));
    chatserver server(&loop, addr, "chatserver");
    server.start();

    // 每 1s 检查一次退出信号
    loop.runEvery(1.0, [&loop]() {
        if (g_quit.load())
        {
            chatservice::init()->resetstate();   // online -> offline
            loop.quit();
        }
    });

    loop.loop();
    std::cout << "server exited gracefully" << std::endl;
    return 0;
}
