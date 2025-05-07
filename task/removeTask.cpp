#include "removeTask.h"

// 构造函数
removeTask::removeTask(std::shared_ptr<std::mutex> clientsMutex, std::unordered_map<int, std::string> &clients, std::shared_ptr<std::mutex> brokenClientsMutex, std::queue<int> &brokenClients,std::atomic<int> &personNumber)
    : clientsMutex(clientsMutex), clients(clients), brokenClientsMutex(brokenClientsMutex), brokenClients(brokenClients),personNumber(personNumber)
{
}

// 析构函数
removeTask::~removeTask()
{
}

void removeTask::execute()
{
    std::lock_guard<std::mutex> lock(*clientsMutex);             // 加锁，避免并发出问题
    std::lock_guard<std::mutex> brokenLock(*brokenClientsMutex); // 双锁
    while (!brokenClients.empty())
    {
        int fd = brokenClients.front();
        brokenClients.pop();
        // 从 clients 中删除该客户端
        auto it = clients.find(fd);
        if (it != clients.end())
        {
            clients.erase(it);
            personNumber.fetch_sub(1);
            LOG_INFO("Removed client fd: " + std::to_string(fd));
        }
        else
        {
            LOG_WARN("Attempted to remove unknown client fd: " + std::to_string(fd));
        }

        // 关闭该 socket（如果尚未关闭）
        close(fd);
    }
}