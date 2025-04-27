#include "noticeTask.h"
// 构造函数
noticeTask::noticeTask(int n, std::vector<int> clients) : number(n), clients(clients)
{
}

// 析构函数
noticeTask::~noticeTask()
{
}

void noticeTask::execute()
{
    // 处理客户端消息
    std::string message = std::string("该服务器中还有") + std::to_string(number) + std::string("人。");
    std::string str = JsonHelper::make_json("notice", "server", message).dump();
    for (int i : clients)
    {
        send(i, str.c_str(), str.size(), 0);
    }
}
