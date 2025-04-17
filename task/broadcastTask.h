#ifndef __BROADCASTTASK_H__
#define __BROADCASTTASK_H__
#include "task.h"
#include <vector>
#include <sys/socket.h>
class broadcastTask : public Task
{
public:
    // 构造函数
    broadcastTask(json j,int clientFd,std::vector<int> clients);

    // 析构函数
    ~broadcastTask();

    void execute();

private:
    json j; // 消息
    int clientFd;       // 客户端套接字
    std::vector<int> clients; // 客户端列表
};
#endif // __BROADCASTTASK_H__