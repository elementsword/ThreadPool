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
class Client
{
public:
    // 构造
    Client(int port, const std::string &serverIp);
    // 析构
    ~Client();
    // 连接服务器
    void connectToServer();
    // 发送消息
    void sendMessage(const std::string &message);
    // 接收消息
    std::string receiveMessage();
    // 关闭连接
    void closeConnection();

private:
    int clientSocket;                                  // 客户端套接字
    std::string serverIp;                              // 服务器 IP 地址
    int port;                                          // 服务器端口
    struct sockaddr_in serverAddr;                     // 服务器地址结构体
    void handleError(const std::string &errorMessage); // 错误处理函数
    int epollFd;                                   // epoll 文件描述符
};
#endif