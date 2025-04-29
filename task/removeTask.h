#ifndef __REMOVETASK_H__
#define __REMOVETASK_H__
#include "task.h"
#include <vector>
#include <sys/socket.h>
#include "../log/log.h"
class removeTask : public Task
{
public:
    // 构造函数
    removeTask(std::shared_ptr<std::mutex> clientsMutex,std::unordered_map<int, std::string> &clients);

    // 析构函数
    ~removeTask();

    void execute();

private:
    std::shared_ptr<std::mutex> clientsMutex;     // 要删除的客户端的锁
    std::unordered_map<int, std::string> clients; // 客户端列表
};
#endif // __REMOVETASK_H__