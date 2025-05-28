#ifndef CLIENT_H
#define CLIENT_H
#include <string>
#include <arpa/inet.h>
#include "../mysqlPool/mysqlPool.h"


// 前向声明日志模块和 JSON 工具类（假设是类）
class Logger;
class JsonHelper;

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
    struct sockaddr_in serverAddr;                     // 服务器地址结构体
    std::string username,password;                     // 用户名 密码
    mysqlPool *sqlPool;                                // 数据库连接池对象 
    void handleError(const std::string &errorMessage); // 错误处理函数
    void exitNormal();                                 //优雅退出 
    void sendMessage(const std::string &message);      //发送信息
    void receiveMessage();                             // 接收消息
    void closeConnection();                            // 关闭连接
    bool login();                                      // 登录
    void registerAccount();                            // 注册
    void ui();                                         // ui
    void uploadFile(const std::string &filepath);      //上传

};
#endif //CLIENT_H