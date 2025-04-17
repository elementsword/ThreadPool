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
#define eventsSize 1024
class Server {
public:
    Server(int port, int threadPoolSize);
    ~Server();

    // 启动服务器
    void start();

private:
    int personNumber;                  // 人数 
    int serverSocket;                  // 服务器监听套接字
    int epollFd;                       // epoll 文件描述符
    int port;                          // 监听端口
    ThreadPool threadPool;             // 线程池
    std::mutex clientsMutex;           // 保护客户端列表的锁
    std::vector<int> clients;          // 客户端套接字列表
    void handleNewConnection();        // 处理新连接
    void handleClientMessage(int clientSocket); // 处理客户端消息
    void removeClient(int clientSocket);       // 移除客户端
    void noticeNumber();        // 通知服务器上有多少人
};

#endif