#ifndef SERVER_H
#define SERVER_H

#include <sys/epoll.h>
#include <netinet/in.h>
#include <unistd.h>
#include <string>
#include <vector>
#include <mutex>
#include <arpa/inet.h>
#include <iterator>
#include <algorithm>
#include "../task/task.h"
#include "../task/calculateTask.h"
#include "../task/broadcastTask.h"
#include "../task/noticeTask.h"
#include "../threadpool/threadpool.h"
#include "../json/jsonhelper.h"
#include "../mysqlPool/mysqlPool.h"
#include "../task/handleClientMessageTask.h"
#include "../task/removeTask.h"
#include <sys/eventfd.h>

// 客户端状态结构体
struct ClientStatus
{
    int clientFd;
    std::string status; // 如："connected", "disconnected", "busy" 等
};
#define eventsSize 1024
class Server
{
public:
    Server(int port, int threadPoolSize);
    ~Server();

    // 启动服务器
    void start();

private:
    int personNumber;                                       // 人数
    int serverSocket;                                       // 服务器监听套接字
    int epollFd;                                            // epoll 文件描述符
    int port;                                               // 监听端口
    int eventFd;                                            // 用来接收子线程通知
    ThreadPool threadPool;                                  // 线程池
    mysqlPool *sqlPool;                                     // 数据库连接池对象
    std::shared_ptr<std::mutex> clientsMutex;               // 保护客户端列表的锁
    std::shared_ptr<std::mutex> brokenClientsMutex;         // 保护queue队列
    std::unordered_map<int, std::string> clients;           // 客户端套接字列表
    std::queue<int> brokenClients;                          // 断开的队列
    void handleNewConnection();                             // 处理新连接
    void handleClientMessage(int clientSocket);             // 处理客户端消息
    void removeClient();                                    // 移除客户端
    void noticeNumber();                                    // 通知服务器上有多少人
};

#endif