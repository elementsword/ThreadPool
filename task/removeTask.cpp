#include "removeTask.h"

// 构造函数
removeTask::removeTask(std::shared_ptr<std::mutex> clientsMutex, std::unordered_map<int, std::string> &clients)
    : clientsMutex(clientsMutex), clients(clients)
{
}

// 析构函数
removeTask::~removeTask()
{
}

void removeTask::execute()
{

    std::lock_guard<std::mutex> lock(*clientsMutex); // 加锁，避免并发出问题
    for (const auto &it : clients)
    {
        if (it.second != "")
        {
            
        }
    }

}