#include "noticeTask.h"
// 构造函数
noticeTask::noticeTask(std::string message, std::vector<int> clients) : message(message), clients(clients)
{
}

// 析构函数
noticeTask::~noticeTask()
{
}

void noticeTask::execute()
{
    std::string str = JsonHelper::make_json("server", "notice",message).dump();
    for (int i : clients)
    {
        send(i, str.c_str(), str.size(), 0);
    }
}
