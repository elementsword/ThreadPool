#ifndef CLIENT_H
#define CLIENT_H
#include <string>
#include <vector>
#include <arpa/inet.h>
#include <iostream>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/epoll.h> 
#include "../log/log.h" 
#include "../json/jsonhelper.h"
class Client
{
public:
    // 构造
    Client(int port, const std::string &serverIp);
    // 析构
    ~Client();
    // 连接服务器
    void connectToServer();

private:
    int clientSocket;                                  // 客户端套接字
    std::string serverIp;                              // 服务器 IP 地址
    int port;                                          // 服务器端口
    int epollFd;                                       // epoll 文件描述符
    bool isConnected;                                  //客户端是否连接
    bool isLogin;                                      //是否登录 
    struct sockaddr_in serverAddr;                     // 服务器地址结构体
    std::string username,password;                     // 用户名 密码 
    void handleError(const std::string &errorMessage); // 错误处理函数
    void exitNormal();                                  //优雅退出 
    void sendMessage(const json &message);              //发送信息
    void receiveMessage();                              // 接收消息
    void closeConnection();                             // 关闭连接
    bool login();

};
#endif