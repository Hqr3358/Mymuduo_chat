#pragma once
#include <string>

// 通信协议消息类型（全链路统一使用字符串，避免 数字/字符串 混用导致 json 比较抛异常）
const std::string REGT       = "REGT";        // 注册
const std::string LOGTO      = "LOGTO";       // 登录
const std::string CHAT       = "CHAT";        // 单聊
const std::string ADDFRIEND  = "ADDFRIEND";   // 加好友
const std::string CREATEGROUP= "CREATEGROUP";  // 创建群
const std::string INSERTGROUP= "INSERTGROUP";  // 加入群
const std::string GROUPCHAT  = "GROUPCHAT";   // 群聊
const std::string REMOVEUSER = "REMOVEUSER";  // 退出群
const std::string OFFLINE    = "OFFLINE";     // 主动下线
const std::string ERRNO      = "ERRNO";       // 错误响应
