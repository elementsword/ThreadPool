// clientInfo.h
#ifndef CLIENTINFO_H
#define CLIENTINFO_H
#include <string>

// 客户端信息结构体
struct clientInfo
{
    std::string username; // 用户名
    std::string status;   // 状态，如 "connected"、"disconnected"、"busy"
    std::string salt;   //盐值
    std::string password; //密码 
};

#endif // CLIENTINFO_H