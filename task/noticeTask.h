#ifndef __NOTICETASK_H__
#define __NOTICETASK_H__
#include "task.h"
#include <vector>
#include <sys/socket.h>
class noticeTask : public Task
{
public:
    // 构造函数
    noticeTask(int n, const std::unordered_map<int, std::string> &clients);

    // 析构函数
    ~noticeTask();

    void execute();

private:
    int number;                                   // 消息
    std::unordered_map<int, std::string> clients; // 客户端列表
};
#endif // __BROADCASTTASK_H__