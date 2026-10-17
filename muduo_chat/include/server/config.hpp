#pragma once
#include <string>

// 全局配置（后续可改为读 json/ini 配置文件，这里集中管理避免散落硬编码）
struct Config
{
    // MySQL
    std::string mysql_host     = "localhost";
    std::string mysql_user     = "qiu";
    std::string mysql_passwd   = "1234";
    std::string mysql_db        = "user1";
    unsigned int mysql_port    = 3306;
    int         mysql_poolsize = 4;     // 连接池大小

    // Redis
    std::string redis_host     = "127.0.0.1";
    unsigned int redis_port    = 6379;

    // 业务
    int         idle_timeout_sec = 30;  // 连接空闲超时踢下线
    int         check_interval_sec = 15; // 超时检测周期

    static Config& instance()
    {
        static Config c;
        return c;
    }
};
