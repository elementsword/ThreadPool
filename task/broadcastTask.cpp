#include "broadcastTask.h"

// 构造函数
broadcastTask::broadcastTask(std::string message, int clientFd, std::vector<int> clients):
    message(message), clientFd(clientFd), clients(clients)
{

}

// 析构函数
broadcastTask::~broadcastTask(){

}

void broadcastTask::execute()
{
    for(int i:clients)
    {
        // 发送消息给其他客户端
        if(i != clientFd)
        {
            send(i, message.c_str(), message.size(), 0);
        }
    }
}