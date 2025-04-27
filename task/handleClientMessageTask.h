#ifndef __HANDLECLIENTMESSAGETASK_H__
#define __HANDLECLIENTMESSAGETASK_H__
#include "task.h"
#include <vector>
#include <sys/socket.h>
#include "../log/log.h"
#include "../mysqlPool/mysqlPool.h"
#include <sys/epoll.h>

class handleClientMessageTask : public Task
{
public:
    // 构造函数
    handleClientMessageTask(int clientSocket,std::vector<int> clients,mysqlPool *sqlPool,int epollFd, std::mutex eventMutex,std::vector<int> closeFd);

    // 析构函数
    ~handleClientMessageTask();

    void execute();

private:
    int clientSocket; // socket对象 
    std::vector<int> clients; // 客户端列表
    mysqlPool *sqlPool;   //数据库连接池
    int epollFd; //文件描述符
    std::mutex eventMutex;//eventFd锁
    std::vector<int> closeFd
};
#endif //__HANDLECLIENTMESSAGETASK_H__ 