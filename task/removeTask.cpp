#include "removeTask.h"

// 构造函数
removeTask::removeTask(std::shared_ptr<std::mutex> clientsMutex, std::unordered_map<int, clientInfo> &clients, std::shared_ptr<std::mutex> brokenClientsMutex, std::queue<int> &brokenClients, std::atomic<int> &personNumber, mysqlPool *sqlPool)
    : clientsMutex(clientsMutex), clients(clients), brokenClientsMutex(brokenClientsMutex), brokenClients(brokenClients), personNumber(personNumber), sqlPool(sqlPool)
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
        LOG_INFO("队列长度：" + std::to_string(brokenClients.size()));
        int fd = brokenClients.front();
        brokenClients.pop();
        // 从 clients 中删除该客户端
        auto it = clients.find(fd);
        if (it != clients.end())
        {
            clientInfo removeClient = it->second;
            clients.erase(it);
            personNumber.fetch_sub(1);
            std::shared_ptr<sql::Connection> conn = sqlPool->getConnection();
            std::unique_ptr<sql::PreparedStatement> updatePstmt(
                conn->prepareStatement("UPDATE users SET is_logged_in=0 WHERE username=?"));
            updatePstmt->setString(1, removeClient.username);
            updatePstmt->execute();
            LOG_INFO("Removed client fd: " + std::to_string(fd));
            LOG_INFO("Set is_logged_in = 0 for user: " + removeClient.username);
        }
        else
        {
            LOG_WARN("Attempted to remove unknown client fd: " + std::to_string(fd));
        }

        // 关闭该 socket（如果尚未关闭）
        close(fd);
    }
}