#ifndef __HANDLECLIENTMESSAGETASK_H__
#define __HANDLECLIENTMESSAGETASK_H__
#include "task.h"
#include <vector>
#include <sys/socket.h>
#include "../log/log.h"
#include <sys/epoll.h>
enum class MessageType {
    EXIT,
    LOGIN,
    TEXT,
    REGISTER,
    UNKNOWN
};

class handleClientMessageTask : public Task
{
public:
    // 构造函数
    handleClientMessageTask(int clientSocket, mysqlPool *sqlPool, int epollFd,int eventFd,std::shared_ptr<std::mutex> clientsMutex,std::unordered_map<int, clientInfo> &clients,std::shared_ptr<std::mutex> brokenClientsMutex,std::queue<int> &brokenClients);

    // 析构函数
    ~handleClientMessageTask();

    void execute();

private:
    int clientSocket;         // socket对象
    mysqlPool *sqlPool;       // 数据库连接池
    int epollFd;              // 文件描述符
    int eventFd;              //事件fd 
    std::shared_ptr<std::mutex> clientsMutex ; //要删除的客户端的锁
    std::unordered_map<int, clientInfo> &clients; // 客户端列表
    std::shared_ptr<std::mutex> brokenClientsMutex;         // 保护queue队列
    std::queue<int> &brokenClients;          //断开的队列
    void notifyClientExit(int clientSocket,std::shared_ptr<std::mutex> brokenClientsMutex, std::queue<int>& brokenClients, int eventFd);
    MessageType stringToMessageType(const std::string& typeStr);       //str 转换 MessageType
};
#endif //__HANDLECLIENTMESSAGETASK_H__