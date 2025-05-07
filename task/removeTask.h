#ifndef __REMOVETASK_H__
#define __REMOVETASK_H__
#include "task.h"
#include <vector>
#include <sys/socket.h>
#include "../log/log.h"
#include <queue>
#include <unistd.h>
class removeTask : public Task
{
public:
    // 构造函数
    removeTask(std::shared_ptr<std::mutex> clientsMutex,std::unordered_map<int, clientInfo> &clients,std::shared_ptr<std::mutex> brokenClientsMutex,std::queue<int> &brokenClients,std::atomic<int> &personNumber,mysqlPool *sqlPool);

    // 析构函数
    ~removeTask();

    void execute();

private:
    std::shared_ptr<std::mutex> clientsMutex;     // 要删除的客户端的锁
    std::unordered_map<int, clientInfo> &clients; // 客户端列表
    std::shared_ptr<std::mutex> brokenClientsMutex;// 保护queue队列
    std::queue<int> &brokenClients;     // 断开的队列
    std::atomic<int> &personNumber;     // 人数
    mysqlPool *sqlPool;       // 数据库连接池
};
#endif // __REMOVETASK_H__