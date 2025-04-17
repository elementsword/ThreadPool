#include "broadcastTask.h"

// 构造函数
broadcastTask::broadcastTask(json j, int clientFd, std::vector<int> clients) : j(j), clientFd(clientFd), clients(clients)
{
}

// 析构函数
broadcastTask::~broadcastTask()
{
}

void broadcastTask::execute()
{
    std::string str = j.dump();
    for (int i : clients)
    {
        // 发送消息给其他客户端
        if (i != clientFd)
        {
            send(i, str.c_str(), str.size(), 0);
        }
    }
}