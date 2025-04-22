#ifndef __LOGINTASK_H__
#define __LOGINTASK_H__

#include "task.h"

class loginTask : public Task
{
public:
    // 构造函数
    loginTask(json j,int clientSocket);

    // 析构函数
    ~loginTask();

    void execute();

private:
    json j; // 消息
    int clientSocket; //客户端
};
#endif // __LOGINTASK_H__
