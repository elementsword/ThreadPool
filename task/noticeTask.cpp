#include "noticeTask.h"
// 构造函数
noticeTask::noticeTask(std::string message, std::vector<int> clients):
    message(message),clients(clients)
{

}

// 析构函数
noticeTask::~noticeTask(){

}

void noticeTask::execute()
{
    for(int i:clients)
    {
            send(i, message.c_str(), message.size(), 0);
    }
}
