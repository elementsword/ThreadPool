#ifndef __NOTICETASK_H__
#define __NOTICETASK_H__
#include "task.h"
#include <vector>
#include <sys/socket.h>
class noticeTask : public Task
{
public:
    // 构造函数
    noticeTask(std::string message,std::vector<int> clients);

    // 析构函数
    ~noticeTask();

    void execute();

private:
    std::string message; // 消息
    std::vector<int> clients; // 客户端列表
};
#endif // __BROADCASTTASK_H__